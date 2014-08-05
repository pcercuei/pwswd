
#ifndef BACKENDS_H
#define BACKENDS_H

#ifdef BACKEND_VOLUME
#include "volume/vol_backend.h"
#endif

#ifdef BACKEND_BRIGHTNESS
#include "brightness/bright_backend.h"
#endif

#ifdef BACKEND_REBOOT
#include "reboot/reboot_backend.h"
#endif

#ifdef BACKEND_POWEROFF
#include "poweroff/poweroff_backend.h"
#endif

#ifdef BACKEND_SCREENSHOT
#include "screenshot/screenshot_backend.h"
#endif

#ifdef BACKEND_TVOUT
#include "tvout/tvout_backend.h"
#endif

#ifdef BACKEND_SUSPEND
#include "suspend/suspend_backend.h"
#endif

#ifdef BACKEND_KILL
#include "kill/kill_backend.h"
#endif

#ifdef BACKEND_RATIOMODE
#include "ratiomode/ratiomode.h"
#endif

#define FRAMEBUFFER "/dev/fb0"

#endif
