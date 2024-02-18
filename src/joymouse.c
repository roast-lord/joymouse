#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <linux/input-event-codes.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xfixes.h>

char MOUSE_PATH[] = "/dev/input/event4";

const int MOUSE_SENSITIVITY = 1600;

int sz = 0;
int mouse_fd = 0;
const int sleep_time_ms = 2000;

const int BTN_SIDE_1 = BTN_EXTRA;
const int BTN_SIDE_2 = BTN_SIDE;

static bool hidden = false;

Display *display;

int num_screens;
Window *roots; // root of each screen

// screen and root window the cursor is in, as set in cursor_find
int active_screen;
Window active_root;

// the screen to use with the onescreen option
int default_screen;

void cursor_show(void)
{
    if (!hidden)
        return;

    printf("Showing the cursor back.\n");
    XFixesShowCursor(display, active_root);
    XFlush(display);
    hidden = false;
}

void cursor_hide(void)
{
    if (hidden)
        return;

    printf("Hiding your mouse cursor. It will be back when you exit the program.\n");
    XFixesHideCursor(display, active_root);
    XFlush(display);
    hidden = true;
}

void cursor_find(Window *child, int *root_x, int *root_y)
{
    Window root;
    int win_x, win_y;
    unsigned int mask;
    int screen;

    for (screen = 0; screen < num_screens; screen++)
    {
        if (XQueryPointer(display, roots[screen], &root, child, root_x, root_y, &win_x, &win_y, &mask))
        {
            active_screen = screen;
            active_root = roots[screen];
            break;
        }
    }
}

static void display_init()
{
    int screen;
    Window child;
    int root_x, root_y;

    num_screens = ScreenCount(display);
    roots = calloc(sizeof(Window), num_screens);
    if (roots == NULL)
        printf("Failed to allocate root windows.");

    for (screen = 0; screen < num_screens; screen++)
        roots[screen] = XRootWindow(display, screen);
    active_screen = DefaultScreen(display);
    active_root = RootWindow(display, active_screen);
    default_screen = active_screen;
    cursor_find(&child, &root_x, &root_y);
}

void send_sync_event(int gamepad_fd, struct input_event gamepad_event)
{
    memset(&gamepad_event, 0, sizeof(struct input_event));
    gamepad_event.type = EV_SYN;
    gamepad_event.code = 0;
    gamepad_event.value = 0;

    if (write(gamepad_fd, &gamepad_event, sizeof(struct input_event)) < 0)
    {
        printf("error writing gamepad sync event\n");
    }
}

void send_mouse_event(int gamepad_fd, struct input_event gamepad_event, int TYPE, int CODE, int VALUE)
{
    memset(&gamepad_event, 0, sizeof(struct input_event));
    gamepad_event.type = TYPE;
    gamepad_event.code = CODE;
    gamepad_event.value = VALUE;

    if (write(gamepad_fd, &gamepad_event, sizeof(struct input_event)) < 0)
    {
        printf("Error writing mouse event to gamepad!\n");
    }
}

static void on_exit_hook(void)
{
    printf("Exiting.\n");
    cursor_show();
    ioctl(mouse_fd, EVIOCGRAB, 1);
    close(mouse_fd);

    if (display != NULL)
        XCloseDisplay(display);
}

int main(int argc, char *argv[])
{
    atexit(on_exit_hook);
    // TODO: Map keyboard R to X+A
    sleep(1);
    char mouse_name[256] = "Unknown";
    mouse_fd = open(MOUSE_PATH, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (mouse_fd == -1)
    {
        printf("Failed to open mouse, make shure to run this as root.\n");
        exit(1);
    }

    display = XOpenDisplay(NULL);

    if (display == NULL)
    {
        printf("Failed to connect to the X server.\n");
        exit(1);
    }

    display_init();
    cursor_hide();

    ioctl(mouse_fd, EVIOCGNAME(sizeof(mouse_name)), mouse_name);
    printf("Reading mouse inputs from : %s \n", mouse_name);

    struct input_event mouse_event;
    int gamepad_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (gamepad_fd < 0)
    {
        printf("Opening of input failed! \n");
        return 1;
    }

    // setting Gamepad keys
    ioctl(gamepad_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_A);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_B);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_X);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_Y);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_TL);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_TR);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_TL2);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_TR2);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_START);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_SELECT);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_THUMBL);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_THUMBR);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_DPAD_UP);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_DPAD_DOWN);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_DPAD_LEFT);
    ioctl(gamepad_fd, UI_SET_KEYBIT, BTN_DPAD_RIGHT);

    // setting Gamepad thumbsticks
    ioctl(gamepad_fd, UI_SET_EVBIT, EV_ABS);
    ioctl(gamepad_fd, UI_SET_ABSBIT, ABS_X);
    ioctl(gamepad_fd, UI_SET_ABSBIT, ABS_Y);
    ioctl(gamepad_fd, UI_SET_ABSBIT, ABS_RX);
    ioctl(gamepad_fd, UI_SET_ABSBIT, ABS_RY);
    ioctl(gamepad_fd, UI_SET_ABSBIT, ABS_TILT_X);
    ioctl(gamepad_fd, UI_SET_ABSBIT, ABS_TILT_Y);

    struct uinput_user_dev uidev;

    memset(&uidev, 0, sizeof(uidev));
    // Name of Gamepad
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Virtual Gamepad");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x3;
    uidev.id.product = 0x3;
    uidev.id.version = 2;
    // Parameters of thumbsticks
    uidev.absmax[ABS_X] = 32767;
    uidev.absmin[ABS_X] = -32768;
    uidev.absfuzz[ABS_X] = 0;
    uidev.absflat[ABS_X] = 15;
    uidev.absmax[ABS_Y] = 32767;
    uidev.absmin[ABS_Y] = -32768;
    uidev.absfuzz[ABS_Y] = 0;
    uidev.absflat[ABS_Y] = 15;
    uidev.absmax[ABS_RX] = 512;
    uidev.absmin[ABS_RX] = -512;
    uidev.absfuzz[ABS_RX] = 0;
    uidev.absflat[ABS_RX] = 16;
    uidev.absmax[ABS_RY] = 512;
    uidev.absmin[ABS_RY] = -512;
    uidev.absfuzz[ABS_RY] = 0;
    uidev.absflat[ABS_RY] = 16;

    if (write(gamepad_fd, &uidev, sizeof(uidev)) < 0)
    {
        printf("Failed to write! \n");
        return 1;
    }
    if (ioctl(gamepad_fd, UI_DEV_CREATE) < 0)
    {
        printf("Failed to create gamepad! \n");
        return 1;
    }

    sleep(1);

    struct input_event gamepad_event;

    printf("To exit press Ctrl+C\n");
    while (true)
    {
        sz = read(mouse_fd, &mouse_event, sizeof(struct input_event));

        if (sz)
        {
            if (sz != -1)

                switch (mouse_event.type)
                {
                case EV_REL:
                    if (mouse_event.code == REL_X)
                    {
                        int to_write = 0;
                        if (mouse_event.value > 0)
                        {
                            to_write = MOUSE_SENSITIVITY;
                        }
                        if (mouse_event.value < 0)
                        {
                            to_write = -MOUSE_SENSITIVITY;
                        }
                        if (mouse_event.value == 0)
                        {
                            to_write = 0;
                        }
                        send_mouse_event(gamepad_fd, gamepad_event, EV_ABS, ABS_RX, to_write);
                        send_sync_event(gamepad_fd, gamepad_event);
                    }
                    else if (mouse_event.code == REL_Y)
                    {
                        int to_write = 0;
                        if (mouse_event.value > 0)
                        {
                            to_write = MOUSE_SENSITIVITY;
                        }
                        if (mouse_event.value < 0)
                        {
                            to_write = -MOUSE_SENSITIVITY;
                        }
                        if (mouse_event.value == 0)
                        {
                            to_write = 0;
                        }

                        send_mouse_event(gamepad_fd, gamepad_event, EV_ABS, ABS_RY, to_write);
                        send_sync_event(gamepad_fd, gamepad_event);
                    }
                    usleep(sleep_time_ms);
                    break;
                case EV_KEY:
                    if (mouse_event.code == BTN_LEFT)
                    {
                        // Mapping to A
                        send_mouse_event(gamepad_fd, gamepad_event, EV_KEY, BTN_A, mouse_event.value);
                        send_sync_event(gamepad_fd, gamepad_event);
                    }
                    else if (mouse_event.code == BTN_RIGHT)
                    {
                        // Mapping to LT
                        printf("Right click\n");
                    }
                    else if (mouse_event.code == BTN_MIDDLE)
                    {

                        // Scroll up = A
                        // Scroll down = Y
                        printf("Right click\n");
                    }
                    else if (mouse_event.code == BTN_SIDE_1)
                    {
                        // Mapping to X + RB
                        send_mouse_event(gamepad_fd, gamepad_event, EV_KEY, BTN_X, mouse_event.value);
                        send_mouse_event(gamepad_fd, gamepad_event, EV_KEY, BTN_TR, mouse_event.value);
                        send_sync_event(gamepad_fd, gamepad_event);
                    }
                    break;
                }
        }
        send_mouse_event(gamepad_fd, gamepad_event, EV_ABS, ABS_RX, 0);
        send_sync_event(gamepad_fd, gamepad_event);
        send_mouse_event(gamepad_fd, gamepad_event, EV_ABS, ABS_RY, 0);
        send_sync_event(gamepad_fd, gamepad_event);
        usleep(10);
    }
    exit(EXIT_SUCCESS);
}