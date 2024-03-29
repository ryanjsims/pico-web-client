#include "tcp_client.h"

#include <pico/cyw43_arch.h>

#include "lwip/pbuf.h"
#include "lwip/dns.h"
#include "lwip/tcp.h"

tcp_client::tcp_client()
    : port_(0)
    , connected_(false)
    , initialized_(false)
    , sent_len(0)
    , buffer_len(0)
    , tcp_controlblock(nullptr)
    , remote_addr({0})
    , user_receive_callback([](){})
    , user_connected_callback([](){})
    , user_poll_callback([](){})
    , user_closed_callback([](){})
    , user_error_callback([](err_t){})
{
    debug1("Initializing DNS...\n");
    dns_init();
    ip_addr_t dnsserver;
    ip4addr_aton("1.1.1.1", &dnsserver);
    dns_setserver(0, &dnsserver);
    
    debug1("Initializing TCP Client\n");
    initialized_ = init();
}

tcp_client::~tcp_client() {
    trace1("tcp_client dtor entered\n");
    close(ERR_CLSD);
    trace1("tcp_client dtor exited\n");
}

bool tcp_client::init() {
    debug1("Initializing tcp_client\n");
    if(tcp_controlblock != nullptr) {
        error1("tcp_controlblock != null!\n");
        return false;
    }
    tcp_controlblock = tcp_new_ip_type(IPADDR_TYPE_V4);
    if(tcp_controlblock == nullptr) {
        error1("Failed to create tcp control block");
        return false;
    }
    tcp_arg(tcp_controlblock, this);
    tcp_poll(tcp_controlblock, poll_callback, POLL_TIME_S * 2);
    tcp_sent(tcp_controlblock, sent_callback);
    tcp_recv(tcp_controlblock, recv_callback);
    tcp_err(tcp_controlblock, err_callback);
    return true;
}

int tcp_client::available() const {
    return buffer.size();
}

size_t tcp_client::read(std::span<uint8_t> out) {
    return buffer.get(out);
}

bool tcp_client::write(std::span<const uint8_t> data) {
    cyw43_arch_lwip_begin();
    err_t err = tcp_write(tcp_controlblock, data.data(), data.size(), TCP_WRITE_FLAG_COPY);
    cyw43_arch_lwip_end();

    debug("tcp_client::write: tcp_write returned %s\n", tcp_perror(err).c_str());

    return err == ERR_OK;
}

void tcp_client::flush() {
    cyw43_arch_lwip_begin();
    err_t err = tcp_output(tcp_controlblock);
    cyw43_arch_lwip_end();

    debug("tcp_client::flush: tcp_output returned %s\n", tcp_perror(err).c_str());
}

bool tcp_client::connected() const {
    return connected_;
}

bool tcp_client::initialized() const {
    return initialized_;
}

bool tcp_client::connect(ip_addr_t addr, uint16_t port) {
    debug("tcp_client::connect to %s:%d\n", ip4addr_ntoa(&addr), port);
    remote_addr = addr;
    port_ = port;

    return connect();
}

bool tcp_client::connect(std::string addr, uint16_t port) {
    info("tcp_client::connect to %s:%d\n", addr.c_str(), port);
    err_t err = dns_gethostbyname(addr.c_str(), &remote_addr, dns_callback, this);
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

bool tcp_client::connect() {
    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(tcp_controlblock, &remote_addr, port_, connected_callback);
    cyw43_arch_lwip_end();

    return err == ERR_OK;
}

err_t tcp_client::close(err_t reason) {
    err_t err = ERR_OK;
    if (tcp_controlblock != NULL) {
        debug1("Connection closing...\n");
        tcp_arg(tcp_controlblock, NULL);
        tcp_poll(tcp_controlblock, NULL, 0);
        tcp_sent(tcp_controlblock, NULL);
        tcp_recv(tcp_controlblock, NULL);
        tcp_err(tcp_controlblock, NULL);
        err = tcp_close(tcp_controlblock);
        if (err != ERR_OK) {
            error("close failed with code %d, calling abort\n", err);
            tcp_abort(tcp_controlblock);
            err = ERR_ABRT;
        }
        tcp_controlblock = NULL;
    }
    connected_ = false;
    initialized_ = false;
    if(reason == ERR_CLSD) {
        user_closed_callback();
    } else {
        user_error_callback(reason);
    }
    return err;
}

void tcp_client::on_poll(uint8_t interval_seconds, std::function<void()> callback) {
    tcp_poll(tcp_controlblock, poll_callback, interval_seconds * 2);
    user_poll_callback = callback;
}

void tcp_client::dns_callback(const char* name, const ip_addr_t *addr, void* arg) {
    if(addr == nullptr) {
        error1("dns_callback: addr is nullptr\n");
        return;
    }
    info("ip of %s found: %s\n", name, ipaddr_ntoa(addr));
    tcp_client *client = (tcp_client*)arg;
    client->remote_addr = *addr;
    client->connect();
}

err_t tcp_client::poll_callback(void* arg, tcp_pcb* pcb) {
    debug1("poll_callback\n");
    tcp_client *client = (tcp_client*)arg;
    client->user_poll_callback();
    return ERR_OK;
}

err_t tcp_client::sent_callback(void* arg, tcp_pcb* pcb, u16_t len) {
    tcp_client *client = (tcp_client*)arg;
    debug("Sent %d bytes\n", len);
    return ERR_OK;
}

err_t tcp_client::recv_callback(void* arg, tcp_pcb* pcb, pbuf* p, err_t err) {
    tcp_client *client = (tcp_client*)arg;
    debug1("tcp_client::recv_callback\n");
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
            curr = curr->next;
        }
        tcp_recved(pcb, count);
    }
    pbuf_free(p);

    client->user_receive_callback();

    return ERR_OK;
}

void tcp_client::err_callback(void* arg, err_t err) {
    tcp_client *client = (tcp_client*)arg;
    error("TCP error: code %s\n", tcp_perror(err).c_str());
    client->clear_pcb();
    client->close(err);
}

err_t tcp_client::connected_callback(void* arg, tcp_pcb* pcb, err_t err) {
    tcp_client *client = (tcp_client*)arg;
    debug1("tcp_client::connected_callback\n");
    if(err != ERR_OK) {
        error("connect failed with error code %s\n", tcp_perror(err).c_str());
        return client->close(err);
    }
    client->connected_ = true;
    client->user_connected_callback();
    return ERR_OK;
}

std::string tcp_perror(err_t err) {
    switch(err) {
    case ERR_ABRT:
        return "ERR_ABRT";
    case ERR_ALREADY:
        return "ERR_ALREADY";
    case ERR_ARG:
        return "ERR_ARG";
    case ERR_BUF:
        return "ERR_BUF";
    case ERR_CLSD:
        return "ERR_CLSD";
    case ERR_CONN:
        return "ERR_CONN";
    case ERR_IF:
        return "ERR_IF";
    case ERR_INPROGRESS:
        return "ERR_INPROGRESS";
    case ERR_ISCONN:
        return "ERR_ISCONN";
    case ERR_MEM:
        return "ERR_MEM";
    case ERR_OK:
        return "ERR_OK";
    case ERR_RST:
        return "ERR_RST";
    case ERR_RTE:
        return "ERR_RTE";
    case ERR_TIMEOUT:
        return "ERR_TIMEOUT";
    case ERR_USE:
        return "ERR_USE";
    case ERR_VAL:
        return "ERR_VAL";
    case ERR_WOULDBLOCK:
        return "ERR_WOULDBLOCK";
    default:
        return "ERR not recognized!";
    }
}
