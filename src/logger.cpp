#include <logger.h>

void __dump_ascii_info(const uint8_t *bptr, uint start, uint end) {
    uint i = end;
    while((i & 0xf) != 0) {
        if((i & 0x7) == 0) {
            info_cont1(" ");
        }
        info_cont1("   ");
        i++;
    }
    info_cont1("\t");
    for(int j = start; j < end; j++) {
        if(std::isprint(bptr[j])) {
            info_cont("%c ", bptr[j]);
        } else {
            info_cont1(". ");
        }
    }
}

void __dump_ascii_debug(const uint8_t *bptr, uint start, uint end) {
    uint i = end;
    while((i & 0xf) != 0) {
        if((i & 0x7) == 0) {
            debug_cont1(" ");
        }
        debug_cont1("   ");
        i++;
    }
    debug_cont1("\t");
    for(int j = start; j < end; j++) {
        if(std::isprint(bptr[j])) {
            debug_cont("%c ", bptr[j]);
        } else {
            debug_cont1(". ");
        }
    }
}

void dump_bytes_info(const uint8_t *bptr, uint32_t len) {
    unsigned int i = 0, line_start = 0;
    info("Dumping %d bytes - 0x%08x to 0x%08x", len, bptr, bptr+len);
    for (i = 0; i < len;) {
        if ((i & 0x0f) == 0) {
            __dump_ascii_info(bptr, line_start, i);
            info_cont1("\n");
            line_start = i;
        } else if ((i & 0x07) == 0) {
            info_cont1(" ");
        }
        info_cont("%02x ", bptr[i++]);
    }
    __dump_ascii_info(bptr, line_start, i);
    info_cont1("\n");
}

void dump_bytes(const uint8_t *bptr, uint32_t len) {
    dump_bytes_info(bptr, len);
}

void dump_bytes_debug(const uint8_t *bptr, uint32_t len) {
    unsigned int i = 0, line_start = 0;
    debug("Dumping %d bytes - 0x%08x to 0x%08x", len, bptr, bptr+len);
    for (i = 0; i < len;) {
        if ((i & 0x0f) == 0) {
            __dump_ascii_debug(bptr, line_start, i);
            debug_cont1("\n");
            line_start = i;
        } else if ((i & 0x07) == 0) {
            debug_cont1(" ");
        }
        debug_cont("%02x ", bptr[i++]);
    }
    __dump_ascii_debug(bptr, line_start, i);
    debug_cont1("\n");
}