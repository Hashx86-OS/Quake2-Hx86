#include "data/Archive.h"

#include <Hx86/Hx86.h>
#include <Hx86/Hgui/Hgui.h>
#include <stdio.h>
#include <string.h>

HX86_DECLARE_APP(HX86_APP_GUI);

void Qcommon_Init(int argc, char *argv[]);
void Qcommon_Frame(int msec);
int Sys_Milliseconds(void);
void Sys_Sleep(void);

static char *build_argv[2];

extern void Cbuf_AddText(char *text);

static const char* kBaseDir = "/baseq2";
static Window* g_dialog = nullptr;
static Label* g_progressLabel = nullptr;
static Button* g_downloadButton = nullptr;
static Button* g_exitButton = nullptr;
static volatile bool g_installDone = false;
static bool g_busy = false;

static void OnExitClicked() {
    syscall_exit_group(1);
}

static void SetProgress(const char* text) {
    if (g_progressLabel != nullptr) g_progressLabel->setText(text);
}

static void DataInstallProgress(const char* text) {
    SetProgress(text);
}

static void OnDownloadClicked() {
    if (g_busy) return;

    g_busy = true;
    if (g_downloadButton != nullptr) g_downloadButton->setEnabled(false);
    if (g_exitButton != nullptr) g_exitButton->setEnabled(false);
    SetProgress("Starting download...");

    char error[224];
    if (!ArchiveInstallSharewareData(kBaseDir, DataInstallProgress, error, sizeof(error))) {
        char message[256];
        if (error[0] != '\0') {
            sprintf(message, "Download failed: %s", error);
        } else {
            strcpy(message, "Download failed.");
        }
        printf("[Quake2] %s\n", message);
        SetProgress(message);
        g_busy = false;
        if (g_downloadButton != nullptr) g_downloadButton->setEnabled(true);
        if (g_exitButton != nullptr) g_exitButton->setEnabled(true);
        return;
    }

    printf("[Quake2] Shareware data installed\n");
    SetProgress("Game data installed. Starting Quake II...");
    g_installDone = true;
}

static void ShowMissingPakDialog(const char* pakpath) {
    init_graphics();

    g_dialog = new Window(desktop, 160, 150, 560, 340);
    g_dialog->setWindowTitle("Quake II - Game Data Required");

    Label* title = new Label(g_dialog, 20, 20, 520, 30, "Game data is incomplete.");
    title->setSize(MEDIUM);

    char expectedText[400];
    sprintf(expectedText, "Expected: %s", pakpath);
    Label* expected = new Label(g_dialog, 20, 56, 520, 22, expectedText);
    expected->setSize(TINY);

    Label* explanation =
        new Label(g_dialog, 20, 92, 520, 22, "Quake II needs the shareware demo game data.");
    explanation->setSize(SMALL);

    Label* source = new Label(g_dialog, 20, 120, 520, 22,
                              "Download the original 3.14 demo from gamers.org?");
    source->setSize(SMALL);

    Label* notice =
        new Label(g_dialog, 20, 146, 520, 40,
                  "Hashx86 does not host it. Shareware demo only; no commercial use.");
    notice->setSize(TINY);

    g_downloadButton = new Button(g_dialog, 20, 196, 190, 32, "Download Shareware");
    g_exitButton = new Button(g_dialog, 222, 196, 90, 32, "Exit");

    g_progressLabel = new Label(g_dialog, 20, 244, 520, 60, "");
    g_progressLabel->setSize(TINY);

    g_dialog->AddChild(title);
    g_dialog->AddChild(expected);
    g_dialog->AddChild(explanation);
    g_dialog->AddChild(source);
    g_dialog->AddChild(notice);
    g_dialog->AddChild(g_downloadButton);
    g_dialog->AddChild(g_exitButton);
    g_dialog->AddChild(g_progressLabel);

    g_downloadButton->OnClick(OnDownloadClicked);
    g_exitButton->OnClick(OnExitClicked);

    g_dialog->show();

    while (!g_installDone) {
        syscall_sleep(16);
    }

    // Let the GUI event callback unwind before tearing down the dialog and
    // handing the framebuffer to the engine.
    syscall_sleep(50);
    if (g_dialog != nullptr) {
        g_dialog->Close();
        g_dialog = nullptr;
    }
}

extern "C" void _start(void* arg) {
    init_sys(arg);
    asm volatile ("finit");

    char pakpath[64];
    sprintf(pakpath, "%s/pak0.pak", kBaseDir);
    if (!ArchiveSharewareDataInstalled(kBaseDir)) {
        printf("[Quake2] Quake II game data is incomplete\n");
        ShowMissingPakDialog(pakpath);
        if (!ArchiveSharewareDataInstalled(kBaseDir)) {
            printf("[Quake2] Game data is still missing, exiting\n");
            syscall_exit_group(1);
        }
    }

    build_argv[0] = "quake2";
    build_argv[1] = NULL;

    printf("[Quake2] Starting...\n");
    Qcommon_Init(1, build_argv);

    Cbuf_AddText((char*)"bind w +forward\n");
    Cbuf_AddText((char*)"bind s +back\n");
    Cbuf_AddText((char*)"bind a +moveleft\n");
    Cbuf_AddText((char*)"bind d +moveright\n");
    Cbuf_AddText((char*)"bind SPACE +jump\n");
    Cbuf_AddText((char*)"bind ALT +strafe\n");
    Cbuf_AddText((char*)"bind CTRL +speed\n");

    int oldtime = Sys_Milliseconds();
    int frames = 0;
    while (1) {
        int newtime = Sys_Milliseconds();
        int time = newtime - oldtime;
        if (time < 1) time = 1;

        frames++;
        if (frames % 60 == 0) {
            printf("[Quake2] Frame %d, msec = %d\n", frames, time);
        }

        Qcommon_Frame(time);
        Sys_Sleep();
        oldtime = newtime;
    }
}
