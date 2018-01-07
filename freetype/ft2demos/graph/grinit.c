#include "grobjs.h"
#include "grdevice.h"
#include <stdio.h>

#ifdef DEVICE_X11
#include "x11/grx11.h"
#endif

#ifdef DEVICE_OS2_PM
#include "os2/gros2pm.h"
#endif

#ifdef DEVICE_WIN32
#include "win32/grwin32.h"
#endif

#ifdef DEVICE_MAC
#include "mac/grmac.h"
#endif

#ifdef DEVICE_ALLEGRO
#include "allegro/gralleg.h"
#endif

#ifdef DEVICE_BEOS
#include "beos/grbeos.h"
#endif


/**********************************************************************
 *
 * <Function>
 *    grInitDevices
 *
 * <Description>
 *    This function is in charge of initialising all system-specific
 *    devices. A device is responsible for creating and managing one
 *    or more "surfaces". A surface is either a window or a screen,
 *    depending on the system.
 *
 * <Return>
 *    a pointer to the first element of a device chain. The chain can
 *    be parsed to find the available devices on the current system
 *
 * <Note>
 *    If a device cannot be initialised correctly, it is not part of
 *    the device chain returned by this function. For example, if an
 *    X11 device was compiled in the library, it will be part of
 *    the returned device chain only if a connection to the display
 *    could be established
 *
 *    If no driver could be initialised, this function returns NULL.
 *
 **********************************************************************/
int grInitDevices(void)
{
	const grDevice *device;
	
	gr_num_devices = 0;
#define INIT_DEVICE(dev) \
	device = dev; \
	if (device->init_device() == 0 && gr_num_devices < GR_MAX_DEVICES) \
		gr_devices[gr_num_devices++] = device;

#ifdef DEVICE_X11
	INIT_DEVICE(&gr_x11_device);
#endif
#ifdef DEVICE_OS2_PM
	INIT_DEVICE(&gr_os2pm_device);
#endif
#ifdef DEVICE_WIN32
	INIT_DEVICE(&gr_win32_device);
#endif
#ifdef DEVICE_MAC
	INIT_DEVICE(&gr_mac_device);
#endif
#ifdef DEVICE_ALLEGRO
	INIT_DEVICE(&gr_alleg_device);
#endif
#ifdef DEVICE_BEOS
	INIT_DEVICE(&gr_beos_device);
#endif
	return gr_num_devices;
#undef INIT_DEVICE
}


void grDoneDevices(void)
{
	int i;

	for (i = 0; i < gr_num_devices; i++)
	{
		const grDevice *device = gr_devices[i];
		device->done_device();
		gr_devices[i] = 0;
	}

	gr_num_devices = 0;
}
