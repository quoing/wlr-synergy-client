#define _POSIX_C_SOURCE 199309L

#define DEBUG_LVL 3

#if defined(DEBUG_LVL) && DEBUG_LVL > 0
  #define DEBUG(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, ##args)
#else
  #define DEBUG(fmt, args...) /**/
#endif



#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <linux/input.h>
#include <wayland-client.h>
#include <wayland-util.h>
//#include <wayland-client-protocol.h>
//#include <wayland-input-eventcodes.h>
#include "virtual-keyboard-unstable-v1-client-protocol.h"
#include "wlr-virtual-pointer-unstable-v1-client-protocol.h"

#include "synergy-micro-client/uSynergy.h"

char host[] = "192.168.2.124";
char port[] = "24800";
char clientName[] = "PF1ZM3G8";
short int sock = -1;

static struct state {
        // Globals
        char title[255];
        struct wl_display *display;
        struct wl_registry *registry;
        struct wl_seat *seat;
        struct zwp_virtual_keyboard_manager_v1 *vkbd_mgr;
        struct zwlr_foreign_toplevel_manager_v1 *ftl_mgr;
        struct zwlr_virtual_pointer_manager_v1 *vp_mgr;

        bool running;
        void *keyboard;
        void *pointer;

        int32_t clientWidth;
        int32_t clientHeight;

	int buttons_pressed[3]; // synergy only supports 3 buttons
} g_state;

int g_cleanup_run=0;

static const struct wl_output_listener output_listener;
static const struct wl_registry_listener registry_listener;
static const struct wl_callback_listener completed_listener;

static void pointer_move_absolute(struct zwlr_virtual_pointer_v1 *vptr, uint32_t x, uint32_t y);
static void pointer_button(struct zwlr_virtual_pointer_v1 *vptr, uint32_t button, uint16_t state);
static void pointer_press(struct zwlr_virtual_pointer_v1 *vptr, uint32_t button);
static void pointer_release(struct zwlr_virtual_pointer_v1 *vptr, uint32_t button);

int 
timestamp()
{
        struct timespec tp;
        clock_gettime(CLOCK_MONOTONIC, &tp);
        int ms = 1000 * tp.tv_sec + tp.tv_nsec / 1000000;
        return ms;
}



void clientTrace(uSynergyCookie cookie, const char *text)
{
        DEBUG("Client trace: %s", text);
        //fprintf(stderr, text);
}

uSynergyBool clientConnect(uSynergyCookie cookie)
{
        struct sockaddr_in addr;

        DEBUG("Connecting...");

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(host);
        addr.sin_port = htons(atoi(port));

        sock = socket(AF_INET, SOCK_STREAM, 0);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                DEBUG("Connection successfull");
                // set socket non-blocking
                //int val = fcntl(sock, F_GETFL, 0);
                //fcntl(sock, F_SETFL, val | O_NONBLOCK);
                return USYNERGY_TRUE;

                //close(sock);
                //DEBUG("Connection closed");
        }

        return USYNERGY_FALSE;
}

uSynergyBool clientSend(uSynergyCookie cookie, const uint8_t *buffer, int length)
{
        DEBUG("sending data");
        if (write(sock, buffer, length) < 0) {
                DEBUG("Error writting to socket");
                return USYNERGY_FALSE;
        }
        return USYNERGY_TRUE;
}

uSynergyBool clientReceive(uSynergyCookie cookie, uint8_t *buffer, int maxLength, int* outLength)
{
        //DEBUG("reading data");
        int num_bytes = read(sock, buffer, maxLength);
        *outLength = num_bytes;
        DEBUG("data read: %d bytes", num_bytes);
        if (num_bytes <= 0) {
                DEBUG("Error receiving data");
                return USYNERGY_FALSE;
        }
        //DEBUG("Data received");
        return USYNERGY_TRUE;
}

uint32_t clientGetTime()
{
        /*long ms;
        long unsigned int s;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ms = round(ts.tv_nsec / 1.0e6);
        s = ts.tv_sec*1000LL+ms;
        //if (ms > 999) { s++; ms = 0; }
        DEBUG("generated timestamp %lu", s);*/
        return timestamp();
}

void clientSleep(uSynergyCookie cookie, int ms)
{
        DEBUG("Sleeping %d ms", ms);
        struct timespec ts;
        int res;
        if (ms < 0)
        {
                errno = EINVAL;
                return;
        }
        ts.tv_sec = ms / 1000;
        ts.tv_nsec = (ms % 1000) * 1000000;
        do {
                res = nanosleep(&ts, &ts);
        } while (res && errno == EINTR);
}

static void clientMouse(uSynergyCookie cookie, uint16_t x, uint16_t y, int16_t wheelx, int16_t wheely, uSynergyBool btnLeft, uSynergyBool btnRight, uSynergyBool btnMid)
{
        DEBUG("mouse: pos: %d %d, wheel: %d %d, buttons: %d %d %d ", x, y, wheelx, wheely, btnLeft, btnRight, btnMid);
        g_state.running = true;
        pointer_move_absolute(g_state.pointer, (uint32_t)x, (uint32_t)y);
	if (btnLeft != g_state.buttons_pressed[0]) {
		g_state.buttons_pressed[0] = (btnLeft==0)?WL_POINTER_BUTTON_STATE_RELEASED:WL_POINTER_BUTTON_STATE_PRESSED;
		pointer_button(g_state.pointer, BTN_LEFT, g_state.buttons_pressed[0]);
	}
	if (btnRight != g_state.buttons_pressed[1]) {
		g_state.buttons_pressed[1] = (btnRight==0)?WL_POINTER_BUTTON_STATE_RELEASED:WL_POINTER_BUTTON_STATE_PRESSED;
		pointer_button(g_state.pointer, BTN_RIGHT, g_state.buttons_pressed[1]);
	}
	if (btnMid != g_state.buttons_pressed[2]) {
		g_state.buttons_pressed[2] = (btnMid==0)?WL_POINTER_BUTTON_STATE_RELEASED:WL_POINTER_BUTTON_STATE_PRESSED;
		pointer_button(g_state.pointer, BTN_MIDDLE, g_state.buttons_pressed[2]);
	}
        struct wl_callback *callback =  wl_display_sync(g_state.display);
        wl_callback_add_listener(callback, &completed_listener, &g_state);
}

static void clientKeyboard(uSynergyCookie cookie, uint16_t key, uint16_t modifiers, uSynergyBool down, uSynergyBool repeat)
{
        DEBUG("key: %d, mods: %d, down: %d, repeat: %d", key, modifiers, down, repeat);
}


static void output_handle_geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char *make, const char *model, int32_t transform)
{
        //struct *output = data;
        DEBUG("X: %d, Y: %d, transform: %d", x, y, transform);
}

static void output_handle_mode(void *data, struct wl_output *wl_output,
                uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
        DEBUG("width: %d, height: %d", width, height);
        struct state *state = data;
        state->clientWidth = width;
        state->clientHeight = height;
	DEBUG("state: w: %d", state->clientWidth);
}

static void output_handle_done(void *data, struct wl_output *wl_output) {
        //no-op
}

static void output_handle_scale(void * data, struct wl_output *wl_output,
                int32_t factor) {
        DEBUG("factor: %d", factor);
}

static void global_registry_handler(void *data, struct wl_registry *registry,
                uint32_t id, const char *interface,
                uint32_t version) {
        DEBUG("Interface: %s", interface);
        struct state *state = data;

        // bind wl_output
        if (strcmp(interface, "wl_output") == 0) {
                /*struct wl_ctx *wl_ctx = (struct wl_ctx*)data;
                struct output_t *output = malloc(sizeof(struct output_t));
                output->wl_ctx = wl_ctx;
                output->id = id;*/
                //output->output =
                struct wl_output *output = wl_registry_bind(registry, id, &wl_output_interface, version);
                //wl_list_insert(&wl_ctx->outputs, &output->link);
                wl_output_add_listener(output, &output_listener, &g_state);
        }
        // Bind wl_seat
        if (strcmp(interface, wl_seat_interface.name) == 0) {
                state->seat = wl_registry_bind(registry, id, &wl_seat_interface, 7);
        }
        // Bind zwlr_virtual_pointer_manager_v1
        if (strcmp(interface, zwlr_virtual_pointer_manager_v1_interface.name) == 0) {
                state->vp_mgr = wl_registry_bind(
                        registry, id, &zwlr_virtual_pointer_manager_v1_interface, 2
                );
        }
        // Bind zwp_virtual_keyboard_manager_v1
        if (strcmp(interface, zwp_virtual_keyboard_manager_v1_interface.name) == 0) {
                state->vkbd_mgr = wl_registry_bind(
                        registry, id, &zwp_virtual_keyboard_manager_v1_interface, 1
                );
        }
}

static void global_registry_remover(void *data, struct wl_registry *registry,
                uint32_t id) {}

/*static void
pointer_move(struct zwlr_virtual_pointer_v1 *vptr, wl_fixed_t dx, wl_fixed_t dy)
{
        DEBUG("dx: %d, dy: %d", dx, dy);
        if (!dx && !dy) {
                return;
        } else {

                zwlr_virtual_pointer_v1_motion(vptr, timestamp(), dx, dy);
                zwlr_virtual_pointer_v1_frame(vptr);
        }
}*/

static void pointer_button(struct zwlr_virtual_pointer_v1 *vptr, uint32_t button, uint16_t state)
{
	zwlr_virtual_pointer_v1_button(vptr, timestamp(), button, state);
	zwlr_virtual_pointer_v1_frame(vptr);
}



static void pointer_press(struct zwlr_virtual_pointer_v1 *vptr, uint32_t button)
{
	zwlr_virtual_pointer_v1_button(vptr, timestamp(), button, WL_POINTER_BUTTON_STATE_PRESSED);
	zwlr_virtual_pointer_v1_frame(vptr);
}

static void pointer_release(struct zwlr_virtual_pointer_v1 *vptr, uint32_t button)
{
	zwlr_virtual_pointer_v1_button(vptr, timestamp(), button, WL_POINTER_BUTTON_STATE_RELEASED);
	zwlr_virtual_pointer_v1_frame(vptr);
}

static void pointer_move_absolute(struct zwlr_virtual_pointer_v1 *vptr, uint32_t x, uint32_t y)
{
        DEBUG("x: %d, y: %d", x, y);
        if (!x && !y) {
                return;
        } else {
                //zwlr_virtual_pointer_v1_motion_absolute(struct zwlr_virtual_pointer_v1 *zwlr_virtual_pointer_v1, uint32_t time, uint32_t x, uint32_t y, uint32_t x_extent, uint32_t y_extent)
                zwlr_virtual_pointer_v1_motion_absolute(vptr, timestamp(), x, y, g_state.clientWidth, g_state.clientHeight);
                zwlr_virtual_pointer_v1_frame(vptr);
        }
}

static void complete_pointer(void *data, struct wl_callback *callback, uint32_t serial)
{
        struct state *state = data;
        wl_callback_destroy(callback);
        state->running = false;
        //destroy_pointer(state);
        return;
}


static const struct wl_output_listener output_listener = {
        .geometry = output_handle_geometry,
        .mode = output_handle_mode,
        .done = output_handle_done,
        .scale = output_handle_scale,
};

static const struct wl_registry_listener registry_listener = {
        global_registry_handler, global_registry_remover
};

static const struct wl_callback_listener completed_listener = {
        .done = complete_pointer
};






void
cleanup(void)
{
        DEBUG("cleanup");
        if (g_cleanup_run < 1)
        {
                g_cleanup_run = 1;
                wl_registry_destroy(g_state.registry);
                wl_display_disconnect(g_state.display);
                return;
        }
        DEBUG("Cleanup already started once");
        return;
}

void 
signalIntHandler(int signum)
{
        exit(0);
}

int main(void)
{
        atexit(cleanup);
        signal(SIGINT, signalIntHandler);

        DEBUG("wlr_synergy_client starting");

        // getting wayland screensize!
        g_state.display = wl_display_connect(NULL);
        if (g_state.display == NULL) {
                DEBUG("failed to create display");
                return -1;
        }
        g_state.registry = wl_display_get_registry(g_state.display);
        wl_registry_add_listener(g_state.registry, &registry_listener, &g_state);

        wl_display_dispatch(g_state.display);
        wl_display_roundtrip(g_state.display);

        // we should have all deviuces (seat, vkbd and vpointer)
        assert(g_state.seat);
        assert(g_state.vkbd_mgr);
        assert(g_state.vp_mgr);

        /*struct output_t *out, *tmp;
        wl_list_for_each_safe(out, tmp, &wl_ctx.outputs, link) {
                wl_output_destroy(out->output);
                wl_list_remove(&out->link);
                free(out);
        }*/

        g_state.pointer = zwlr_virtual_pointer_manager_v1_create_virtual_pointer(
                g_state.vp_mgr, g_state.seat
        );
        g_state.keyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(
                g_state.vkbd_mgr, g_state.seat
        );

        assert(g_state.pointer);
        assert(g_state.keyboard);

        uSynergyContext ctx;

        uSynergyInit(&ctx);

        ctx.m_clientName=clientName;

	//DEBUG("Width: %d", g_state.clientWidth);
        ctx.m_clientWidth = g_state.clientWidth;
        ctx.m_clientHeight = g_state.clientHeight;

        ctx.m_connectFunc=&clientConnect;
        ctx.m_sendFunc=&clientSend;
        ctx.m_receiveFunc=&clientReceive;
        ctx.m_getTimeFunc=&clientGetTime;
        ctx.m_sleepFunc=&clientSleep;

        ctx.m_mouseCallback=&clientMouse;
        ctx.m_keyboardCallback=&clientKeyboard;

        ctx.m_traceFunc=&clientTrace;

        while (1) {
                uSynergyUpdate(&ctx);

                while (g_state.running)
                {
                        DEBUG("Running");
                        if (wl_display_dispatch(g_state.display) < 0)
                        {
                                break;
                        }
                }
        }
        return 0;
}

