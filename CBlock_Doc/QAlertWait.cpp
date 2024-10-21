#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine,
    int iCmdShow) {
  MessageBox(NULL,
      //   "QSpice simulation is complete."
      //   "\nClose this window to return to QSpice simulation.",
      "QSpice simulation is complete.",
      //   "QAlert", MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);
      //   "QAlert", MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
      "QAlert", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);

  return 0;
}