#include "udp_client.h"

#include <pico/cyw43_arch.h>

#include "lwip/pbuf.h"
#include "lwip/dns.h"
#include "lwip/udp.h"
#include "logger.h"

udp_client::udp_client()
    : initialized_(false)
    , connected_(false)
    , buffer_len(0)
    , sent_len(0)
    , port(0)
    , remote_addr({0})
    , udp_controlblock(nullptr)
    , user_receive_callback(nullptr)
    , user_connected_callback(nullptr)
{
    info1("Initializing DNS...\n");
    dns_init();
    ip_addr_t dnsserver;
    ip4addr_aton("1.1.1.1", &dnsserver);
    dns_setserver(0, &dnsserver);
    
    info1("Initializing UDP Client\n");
    initialized_ = init();
}

udp_client::~udp_client() {
    if(udp_controlblock) {
        udp_remove(udp_controlblock);
    }
}

bool udp_client::init() {
    debug1("Initializing udp_client\n");
    if(udp_controlblock != nullptr) {
        error1("udp_controlblock != null!\n");
        return false;
    }
    udp_controlblock = udp_new_ip_type(IPADDR_TYPE_V4);
    if(udp_controlblock == nullptr) {
        error1("Failed to create tcp control block");
        return false;
    }
    udp_recv(udp_controlblock, recv_callback, this);
    return true;
}

int udp_client::available() const {
    return buffer.size();
}

size_t udp_client::read(std::span<uint8_t> out) {
    return buffer.get(out);
}

bool udp_client::write(std::span<const uint8_t> data) {
    if(data.size() > UINT16_MAX) {
        return false;
    }
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, data.size(), PBUF_RAM);
    if(p == nullptr) {
        return false;
    }
    memcpy(p->payload, data.data(), data.size());
    err_t err = udp_send(udp_controlblock, p);
    pbuf_free(p);
    cyw43_arch_lwip_end();

    return err == ERR_OK;
}

bool udp_client::connect() {
    cyw43_arch_lwip_begin();
    err_t err = udp_connect(udp_controlblock, &remote_addr, port);
    cyw43_arch_lwip_end();

    connected_ = err == ERR_OK;
    if(connected_) {
        user_connected_callback();
    }
    return connected_;
}

bool udp_client::connect(ip_addr_t addr, uint16_t port) {
    debug("udp_client::connect to %s:%d\n", ip4addr_ntoa(&addr), port);
    remote_addr = addr;
    this->port = port;

    return connect();
}

bool udp_client::connect(std::string addr, uint16_t port) {
    debug("tcp_client::connect to %s:%d\n", addr.c_str(), port);
    err_t err = dns_gethostbyname(addr.c_str(), &remote_addr, dns_callback, this);
    this->port = port;
    if(err == ERR_OK) {
        debug1("No dns lookup needed\n");
        err = connect();
    } else if(err != ERR_INPROGRESS) {
        error("gethostbyname failed with error code %d\n", err);
        return false;
    }

    return err == ERR_OK || err == ERR_INPROGRESS;
}

void udp_client::dns_callback(const char* name, const ip_addr_t *addr, void* arg) {
    if(addr == nullptr) {
        error1("dns_callback: addr is nullptr\n");
        return;
    }
    info("ip of %s found: %s\n", name, ipaddr_ntoa(addr));
    udp_client *client = (udp_client*)arg;
    client->remote_addr = *addr;
    client->connect();
}

void udp_client::recv_callback(void* arg, udp_pcb* pcb, pbuf* p, const ip_addr_t *addr, uint16_t port) {
    udp_client *client = (udp_client*)arg;
    info1("udp_client::recv_callback\n");
    if(p == nullptr) {
        return;
    }

    if(p->tot_len > 0) {
        info("recv'ing %d bytes\n", p->tot_len);
        // Receive the buffer
        size_t count = 0;
        pbuf* curr = p;
        while(curr && !client->buffer.full()) {
            count += client->buffer.put({reinterpret_cast<uint8_t*>(curr->payload), curr->len});
            curr = curr->next;
        }
    }
    pbuf_free(p);

    client->user_receive_callback(addr, port);
}

ip_addr_t udp_client::remote_address() const {
    return remote_addr;
}

bool udp_client::initialized() const {
    return initialized_;
}

bool udp_client::connected() const {
    return connected_;
}

void udp_client::on_receive(std::function<void(const ip_addr_t*, uint16_t)> callback) {
    user_receive_callback = callback;
}

void udp_client::on_connect(std::function<void()> callback) {
    user_connected_callback = callback;
}