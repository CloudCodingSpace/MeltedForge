#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "core/mfutils.h"

typedef struct GLFWwindow GLFWwindow;

typedef struct MFWindowConfig_s {
    f64 x, y;
    i32 width, height;
    bool fullscreen, resizable, centered;
    const char* title;
} MFWindowConfig;

#define MF_WINDOW_DEFAULT_ICON_PATH "mfassets/logo/logo2.png"

typedef struct MFWindow_s MFWindow;

// @brief Creates a MFWindow handle
// @return Returns a pointer to a MFWindow handle
// @param config A MFWindowConfig for user's choice of configurations
MFWindow* mfWindowCreate(MFWindowConfig config);

// @breif Destroys a MFWindow
// @param window A valid MFWindow* which is returned by `mfWindowCreate` function
void mfWindowDestroy(MFWindow* window);

// @brief Sets the icon of the window
// @param window A valid MFWindow* which is returned by `mfWindowCreate` function
// @param width A non-zero u32 indicating the width of the icon
// @param height A non-zero u32 indicating the height of the icon
// @param pixels A valid u8* buffer which has the pixel data of the icon
void mfWindowSetIcon(MFWindow* window, u32 width, u32 height, u8* pixels);

// @brief Updates the window for various things like polling events, callbacks, etc
// @param window A valid MFWindow* which is returned by `mfWindowCreate` function
void mfWindowUpdate(MFWindow* window);

// @brief Closes the window
// @param window A valid MFWindow* which is returned by `mfWindowCreate` function
void mfWindowClose(MFWindow* window);

// @brief Shows the window
// @param window A valid MFWindow* which is returned by `mfWindowCreate` function
void mfWindowShow(MFWindow* window);

// @brief Hides the window
// @param window A valid MFWindow* which is returned by `mfWindowCreate` function
void mfWindowHide(MFWindow* window);

// @brief Informs whether the window is open or not
// @return Returns `true` if the window is open. Or else returns `false`
// @param window A valid MFWindow* which is returned by `mfWindowCreate` function
bool mfIsWindowOpen(MFWindow* window);

// @brief Queries the title of the window
// @return Returns a `const char*` which is the title of the window
// @param window A valid MFWindow* which is returned by `mfWindowCreate` function
const char* mfWindowGetTitle(MFWindow* window);

// @brief Sets the title of the window
// @param window A valid MFWindow* which is returned by `mfWindowCreate` function
void mfWindowSetTitle(MFWindow* window, const char* title);

// @brief Queries the internal backend handle of the window
// @return Returns a GLFWwindow*, which is a internal handle of the window
// @param window A valid MFWindow* which is returned by `mfWindowCreate` function
// @note The returned `GLFWwindow*` must **NOT** be modified in any circumstances. It is read only, mainly for other systems like the renderer
GLFWwindow* mfWindowGetHandle(MFWindow* window);

// @brief Queries the config of the window
// @return Returns a const MFWindowConfig* which is read only
// @param window A valid MFWindow* which is returned by `mfWindowCreate` function
const MFWindowConfig* mfWindowGetConfig(MFWindow* window);

// @brief Queries the size of the MFWindow struct in bytes
// @return Returns a size_t which is the `sizeof(struct MFWindow_s)`
size_t mfWindowGetSizeInBytes(void);

#ifdef __cplusplus
}
#endif