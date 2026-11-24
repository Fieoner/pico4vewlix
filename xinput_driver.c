// Minimal TinyUSB class driver exposing two XInput (Xbox 360 wired) interfaces.
#include <string.h>
#include "tusb.h"
#include "device/usbd_pvt.h"
#include "xinput_driver.h"

#define XINPUT_EP_SIZE 32

typedef struct {
    uint8_t ep_in;
    uint8_t ep_out;
    uint8_t itf_num;
    xinput_report_t in_buf;
    uint8_t out_buf[XINPUT_EP_SIZE];
} xinput_itf_t;

static xinput_itf_t _itf[XINPUT_NUM_PLAYERS];

void xinput_report_init(xinput_report_t *r) {
    memset(r, 0, sizeof(*r));
    r->report_id = 0x00;
    r->report_size = sizeof(xinput_report_t);
}

static xinput_itf_t *find_by_ep(uint8_t ep_addr) {
    for (uint8_t i = 0; i < XINPUT_NUM_PLAYERS; i++) {
        if (_itf[i].ep_in && (_itf[i].ep_in == ep_addr || _itf[i].ep_out == ep_addr)) return &_itf[i];
    }
    return NULL;
}

bool xinput_send(uint8_t player, const xinput_report_t *r) {
    if (player >= XINPUT_NUM_PLAYERS) return false;
    xinput_itf_t *p = &_itf[player];
    if (!p->ep_in || !tud_ready()) return false;
    if (usbd_edpt_busy(0, p->ep_in)) return false;
    p->in_buf = *r;
    return usbd_edpt_xfer(0, p->ep_in, (uint8_t *)&p->in_buf, sizeof(p->in_buf));
}

static void xinput_init(void) {
    memset(_itf, 0, sizeof(_itf));
}

static bool xinput_deinit(void) { return true; }

static void xinput_reset(uint8_t rhport) {
    (void) rhport;
    memset(_itf, 0, sizeof(_itf));
}

static uint16_t xinput_open(uint8_t rhport, tusb_desc_interface_t const *desc_itf, uint16_t max_len) {
    TU_VERIFY(desc_itf->bInterfaceClass == 0xFF && desc_itf->bInterfaceSubClass == 0x5D &&
              desc_itf->bInterfaceProtocol == 0x01, 0);

    // interface (9) + vendor-specific xinput descriptor (17) + 2 endpoints (14)
    uint16_t const drv_len = sizeof(tusb_desc_interface_t) + 17 + 2 * sizeof(tusb_desc_endpoint_t);
    TU_VERIFY(max_len >= drv_len, 0);

    xinput_itf_t *p = NULL;
    for (uint8_t i = 0; i < XINPUT_NUM_PLAYERS; i++) {
        if (_itf[i].ep_in == 0) { p = &_itf[i]; break; }
    }
    TU_VERIFY(p, 0);

    // Skip to the first endpoint descriptor
    uint8_t const *d = tu_desc_next(desc_itf);
    while (tu_desc_type(d) != TUSB_DESC_ENDPOINT) d = tu_desc_next(d);

    TU_ASSERT(usbd_open_edpt_pair(rhport, d, 2, TUSB_XFER_INTERRUPT, &p->ep_out, &p->ep_in), 0);
    p->itf_num = desc_itf->bInterfaceNumber;

    // Arm OUT endpoint so rumble/LED packets from the host are drained and ignored
    usbd_edpt_xfer(rhport, p->ep_out, p->out_buf, sizeof(p->out_buf));
    return drv_len;
}

static bool xinput_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    (void) rhport; (void) stage; (void) request;
    return false; // stall vendor/class control requests
}

static bool xinput_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
    (void) result; (void) xferred_bytes;
    xinput_itf_t *p = find_by_ep(ep_addr);
    if (p && ep_addr == p->ep_out) {
        usbd_edpt_xfer(rhport, p->ep_out, p->out_buf, sizeof(p->out_buf));
    }
    return true;
}

static usbd_class_driver_t const xinput_driver = {
#if CFG_TUSB_DEBUG >= 2
    .name             = "XINPUT",
#endif
    .init             = xinput_init,
    .deinit           = xinput_deinit,
    .reset            = xinput_reset,
    .open             = xinput_open,
    .control_xfer_cb  = xinput_control_xfer_cb,
    .xfer_cb          = xinput_xfer_cb,
    .sof              = NULL,
};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return &xinput_driver;
}
