# JoyMouse
JoyMouse is a simple program that creates a virtual gamepad and maps the mouse movements of the user to the virtual gamepad's right analog stick. Additionally, the program can map other mouse buttons to gamepad buttons, providing a convenient way to control games or applications that require a game controller.

# Features
- Maps mouse movements to the virtual gamepad's right analog stick.
- Supports mapping mouse buttons to gamepad buttons.
- Hides the mouse cursor while the program is running.

# Usage

## Finding your mouse file descriptor:
You will need to edit some variables in the code for the program to work.

First you need to know the corresponding file descriptor for your mouse.
You can find it by running:
```$ ls -l /dev/input/by-id/```
It will show something like this:
> lrwxrwxrwx 1 root root usb-some-microfone -> ../event3
lrwxrwxrwx 1 root root usb-some-mouse-event-mouse -> ../event4
lrwxrwxrwx 1 root root usb-some-keyboard-event-kbd -> ../event5

In this case the mouse events will be at event4, so the corresponding descriptor will be:
> /dev/input/event4

Change the variable MOUSE_PATH in the code to your mouse file descriptor.
## Changing the mouse sensitivity:
Additionally you can change the mouse sensitivity by altering the variable MOUSE_SENSITIVITY to your desired value.


## Compiling
Make sure you have a C compiler in your system, it can be gcc or clang.

After changing the variables of the program, compile it by running:
```$ gcc src/joymouse.c -lX11 -lXi -lXfixes -lev -o joymouse```

## Running
Give execute permission to the binary:
```$ chmod +x joymouse```
Execute the binary with root permissions:
```$ sudo ./joymouse```

# Dependencies
X11 libraries: libX11, libXi, libXfixes