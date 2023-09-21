#include "tcp_tls_client.h"

#include <pico/cyw43_arch.h>

#include "lwip/dns.h"

#include "hardware/structs/rosc.h"
void dump_bytes(const uint8_t *bptr, uint32_t len);
// extern "C" {
//     /* Function to feed mbedtls entropy. May be better to move it to pico-sdk */
//     int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen) {
//         /* Code borrowed from pico_lwip_random_byte(), which is static, so we cannot call it directly */
//         static uint8_t byte;

//         for(int p=0; p<len; p++) {
//             for(int i=0;i<32;i++) {
//                 // picked a fairly arbitrary polynomial of 0x35u - this doesn't have to be crazily uniform.
//                 byte = ((byte << 1) | rosc_hw->randombit) ^ (byte & 0x80u ? 0x35u : 0);
//                 // delay a little because the random bit is a little slow
//                 busy_wait_at_least_cycles(30);
//             }
//             output[p] = byte;
//         }

//         *olen = len;
//         return 0;
//     }
// }

tcp_tls_client::tcp_tls_client()
    : port_(0)
    , connected_(false)
    , initialized_(false)
    , sent_len(0)
    , buffer_len(0)
    , tcp_controlblock(nullptr)
    , remote_addr({0})
    , user_receive_callback([](){})
    , user_connected_callback([](){})
    , user_closed_callback([](err_t){}) {
    if(tls_config == nullptr) {
        debug1("Creating tls_config...\n");
        tls_config = altcp_tls_create_config_client(NULL, 0);
    }
}

bool tcp_tls_client::init() {
    debug1("tcp_tls_config::init\n");
    if(tcp_controlblock != nullptr) {
        error1("tcp_controlblock != null!\n");
        return false;
    }
    tcp_controlblock = altcp_tls_new(tls_config, IPADDR_TYPE_V4);
    if(tcp_controlblock == nullptr) {
        error1("Failed to create tcp control block");
        return false;
    }
    altcp_arg(tcp_controlblock, this);
    altcp_poll(tcp_controlblock, poll_callback, POLL_TIME_S * 2);
    altcp_sent(tcp_controlblock, sent_callback);
    altcp_recv(tcp_controlblock, recv_callback);
    altcp_err(tcp_controlblock, err_callback);

    return true;
}

int tcp_tls_client::available() const {
    return buffer.size();
}

size_t tcp_tls_client::read(std::span<uint8_t> out) {
    return buffer.get(out);
}

bool tcp_tls_client::connected() const {
    return connected_;
}

bool tcp_tls_client::initialized() const {
    return initialized_;
}

bool tcp_tls_client::write(std::span<const uint8_t> data) {
    debug("tcp_tls_client::write data=%p size=%d\n", data.data(), data.size());
    #if LOG_LEVEL <= LOG_LEVEL_DEBUG
    for(int i = 0; i < data.size(); i++) {
        debug_cont("%02x ", data[i]);
    }
    #endif
    debug_cont1("\n");
    cyw43_arch_lwip_begin();
    err_t err = altcp_write(tcp_controlblock, data.data(), data.size(), TCP_WRITE_FLAG_COPY);
    cyw43_arch_lwip_end();

    return err == ERR_OK;
}

err_t tcp_tls_client::close(err_t reason) {
    err_t err = ERR_OK;
    if (tcp_controlblock != NULL) {
        debug1("Connection closing...\n");
        altcp_arg(tcp_controlblock, NULL);
        altcp_poll(tcp_controlblock, NULL, 0);
        altcp_sent(tcp_controlblock, NULL);
        altcp_recv(tcp_controlblock, NULL);
        altcp_err(tcp_controlblock, NULL);
        err = altcp_close(tcp_controlblock);
        if (err != ERR_OK) {
            error("close failed with code %d, calling abort\n", err);
            altcp_abort(tcp_controlblock);
            err = ERR_ABRT;
        }
        tcp_controlblock = NULL;
    }
    connected_ = false;
    initialized_ = false;
    user_closed_callback(reason);
    return err;
}

bool tcp_tls_client::connect(std::string hostname, uint16_t port) {
    info("tcp_tls_client::connect to %s:%d\n", hostname.c_str(), port);
    debug1("Setting mbedtls hostname...\n");
    mbedtls_ssl_context* ssl_context = (mbedtls_ssl_context*)altcp_tls_context(tcp_controlblock);
    debug("ssl_context = %p\n", ssl_context);
    int code = mbedtls_ssl_set_hostname(ssl_context, hostname.c_str());
    debug("mbedtls_ssl_set_hostname rc = %d\n", code);

    err_t err = dns_gethostbyname(hostname.c_str(), &remote_addr, dns_callback, this);
    port_ = port;
    if(err == ERR_OK) {
        debug1("No dns lookup needed\n");
        err = connect();
    } else if(err != ERR_INPROGRESS) {
        error("gethostbyname failed with error code %d\n", err);
        close(err);
        return false;
    }

    return err == ERR_OK || err == ERR_INPROGRESS;
}

bool tcp_tls_client::connect() {
    cyw43_arch_lwip_begin();
    err_t err = altcp_connect(tcp_controlblock, &remote_addr, port_, connected_callback);
    cyw43_arch_lwip_end();

    return err == ERR_OK;
}

void tcp_tls_client::dns_callback(const char* name, const ip_addr_t *addr, void* arg) {
    if(addr == nullptr) {
        error1("dns_callback: addr is nullptr\n");
        return;
    }
    info("ip of %s found: %s\n", name, ipaddr_ntoa(addr));
    tcp_tls_client *client = (tcp_tls_client*)arg;
    client->remote_addr = *addr;
    client->connect();
}

err_t tcp_tls_client::connected_callback(void* arg, altcp_pcb* pcb, err_t err) {
    tcp_tls_client *client = (tcp_tls_client*)arg;
    debug1("tcp_tls_client::connected_callback\n");
    if(err != ERR_OK) {
        error1("connect failed with error code ");
        tcp_perror(err);
        return client->close(err);
    }
    client->connected_ = true;
    client->user_connected_callback();
    return ERR_OK;
}

err_t tcp_tls_client::recv_callback(void* arg, altcp_pcb* pcb, pbuf* p, err_t err) {
    tcp_tls_client *client = (tcp_tls_client*)arg;
    debug1("tcp_tls_client::recv_callback\n");
    if(p == nullptr) {
        // Connection closed
        return client->close(ERR_CLSD);
    }

    if(p->tot_len > 0) {
        debug("recv'ing %d bytes\n", p->tot_len);
        // Receive the buffer
        size_t count = 0;
        pbuf* curr = p;
        while(curr && !client->buffer.full()) {
            count += client->buffer.put({reinterpret_cast<uint8_t*>(curr->payload), curr->len});
            #if LOG_LEVEL <= LOG_LEVEL_TRACE
            for(size_t i = 0; i < curr->len; i++) {
                if(isprint(reinterpret_cast<uint8_t*>(curr->payload)[i])){
                    printf("%c", reinterpret_cast<uint8_t*>(curr->payload)[i]);
                } else {
                    printf("\\x%02x ", reinterpret_cast<uint8_t*>(curr->payload)[i]);
                }
            }
            printf("\n");
            #endif
            curr = curr->next;
        }
        altcp_recved(pcb, count);
    }
    pbuf_free(p);

    client->user_receive_callback();

    return ERR_OK;
}

err_t tcp_tls_client::poll_callback(void* arg, altcp_pcb* pcb) {
    trace1("poll_callback\n");
    tcp_tls_client *client = (tcp_tls_client*)arg;
    client->user_poll_callback();
    return ERR_OK;
}

err_t tcp_tls_client::sent_callback(void* arg, altcp_pcb* pcb, uint16_t len) {
    debug("Sent %d bytes\n", len);
    return ERR_OK;
}

void tcp_tls_client::tcp_perror(err_t err) {
    switch(err) {
    case ERR_ABRT:
        error_cont1("ERR_ABRT\n");
        break;
    case ERR_ALREADY:
        error_cont1("ERR_ALREADY\n");
        break;
    case ERR_ARG:
        error_cont1("ERR_ARG\n");
        break;
    case ERR_BUF:
        error_cont1("ERR_BUF\n");
        break;
    case ERR_CLSD:
        error_cont1("ERR_CLSD\n");
        break;
    case ERR_CONN:
        error_cont1("ERR_CONN\n");
        break;
    case ERR_IF:
        error_cont1("ERR_IF\n");
        break;
    case ERR_INPROGRESS:
        error_cont1("ERR_INPROGRESS\n");
        break;
    case ERR_ISCONN:
        error_cont1("ERR_ISCONN\n");
        break;
    case ERR_MEM:
        error_cont1("ERR_MEM\n");
        break;
    case ERR_OK:
        error_cont1("ERR_OK\n");
        break;
    case ERR_RST:
        error_cont1("ERR_RST\n");
        break;
    case ERR_RTE:
        error_cont1("ERR_RTE\n");
        break;
    case ERR_TIMEOUT:
        error_cont1("ERR_TIMEOUT\n");
        break;
    case ERR_USE:
        error_cont1("ERR_USE\n");
        break;
    case ERR_VAL:
        error_cont1("ERR_VAL\n");
        break;
    case ERR_WOULDBLOCK:
        error_cont1("ERR_WOULDBLOCK\n");
        break;
    }
}

void tcp_tls_client::err_callback(void* arg, err_t err) {
    tcp_tls_client *client = (tcp_tls_client*)arg;
    error1("TCP error: code ");
    tcp_perror(err);
    if (err != ERR_ABRT) {
        client->close(err);
    }
}