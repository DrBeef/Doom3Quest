
extern "C"
{
//set if swap buffers actually does an egl swap
void SDL_SwapBufferPerformsSwap(int value);

//Set option function to call before swap buffer
void SDL_SetSwapBufferCallBack(void (*pt2Func)(void));

//Callback to hide/show keyboard
void SDL_SetShowKeyboardCallBack(void (*pt2Func)(int));

// True if a new egl was created in the last time this was called
int SDL_NewEGLCreated();

}
