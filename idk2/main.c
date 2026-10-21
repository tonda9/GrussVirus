#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <linux/uinput.h>
#include <unistd.h>
#include <linux/input.h>
#include <stdlib.h>

void univerzal(int uinput_soubor, int typ, int kod, int hodnota) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = typ;
    ev.code = kod;
    ev.value = hodnota;
    write(uinput_soubor, &ev, sizeof(ev));
}

void poslaniKlavesy(int uinput_soubor, int kod_klavesy) {
    univerzal(uinput_soubor, EV_KEY, kod_klavesy, 1);
    univerzal(uinput_soubor, EV_SYN, SYN_REPORT, 0);
    univerzal(uinput_soubor, EV_KEY, kod_klavesy, 0);
    univerzal(uinput_soubor, EV_SYN, SYN_REPORT, 0);
}

bool detectionKeyboard(char *vysledna_cesta, size_t maximalni_delka) {

    FILE *fp = fopen("/proc/bus/input/devices", "r");
    if (!fp) {
        perror("Nelze otevřít /proc/bus/input/devices");
        return false;
    }

    char line[255];
    char current_handlers[255] = {0};
    bool is_keyboard = false;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "H: Handlers=", 12) == 0) {
            strncpy(current_handlers, line + 12, sizeof(current_handlers) - 1);
        }
        if (strncmp(line, "B: EV=", 6) == 0) {
            char *ev_val = line + 6;
            ev_val[strcspn(ev_val, "\r\n")] = 0;

            unsigned long long ev = strtoull(ev_val, NULL, 16);
            if ((ev & 0x120013) == 0x120013 || (ev & 0x3) == 0x3) {
                if (strstr(current_handlers, "event") != NULL) {
                    is_keyboard = true;
                }
            }
        }

        if ((strcmp(line, "\n") == 0 || strcmp(line, "\r\n" ) == 0) && is_keyboard) {
            char *token = strtok(current_handlers, " \t\n\r");
            while (token != NULL) {
                if (strncmp(token, "event", 5) == 0) {
                    snprintf(vysledna_cesta, maximalni_delka, "/dev/input/%s", token);
                    fclose(fp);
                    return true;
                }
                token = strtok(NULL, " \t\n\r");
            } // OPRAVENO: Správné uzavření vnitřního while cyklu

            is_keyboard = false;
            current_handlers[0] = '\0';
        }
    }
    fclose(fp);
    return false;
}

void prepsani(int soubor) {
    for (int i = 0; i < 7; ++i) {
        poslaniKlavesy(soubor, KEY_BACKSPACE);
        usleep(10000);
    }
    poslaniKlavesy(soubor, KEY_L);
    poslaniKlavesy(soubor, KEY_I);
    poslaniKlavesy(soubor, KEY_N);
    poslaniKlavesy(soubor, KEY_U);
    poslaniKlavesy(soubor, KEY_X);
}

void setrizeni_a_zapsani(int listCharacteru[7], int pismeno, int soubor) {
    for (int i = 6; i > 0; i--) {
        listCharacteru[i] = listCharacteru[i - 1];
    }
    listCharacteru[0] = pismeno;

    if (listCharacteru[6] == 17 && listCharacteru[5] == 23 && listCharacteru[4] == 49 && listCharacteru[3] == 32 &&
        listCharacteru[2] == 24 && listCharacteru[1] == 17 && listCharacteru[0] == 31) {

        prepsani(soubor);
        memset(listCharacteru, 0, 7 * sizeof(int));
    }
}

int main() {
    int listCharacteru[7] = {0};
    int soubor = open("/dev/uinput", O_WRONLY);
    if (soubor == -1) {
        perror("Nelze otevřít /dev/uinput (spustili jste program jako sudo?)");
        return 1;
    }

    char dev[64];
    if (!detectionKeyboard(dev, sizeof(dev))) {
        printf("Klávesnice nebyla nalezena.\n");
        close(soubor);
        return 1;
    }

    int file_popisovac = open(dev, O_RDONLY);
    if (file_popisovac == -1) {
        perror("Chyba s file_popisovacem, nepodařilo se otevřít soubor klávesnice\n");
        close(soubor);
        return 1;
    }

    ioctl(soubor, UI_SET_EVBIT, EV_KEY);
    ioctl(soubor, UI_SET_KEYBIT, KEY_BACKSPACE);
    ioctl(soubor, UI_SET_KEYBIT, KEY_L);
    ioctl(soubor, UI_SET_KEYBIT, KEY_I);
    ioctl(soubor, UI_SET_KEYBIT, KEY_N);
    ioctl(soubor, UI_SET_KEYBIT, KEY_U);
    ioctl(soubor, UI_SET_KEYBIT, KEY_X);

    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    strcpy(usetup.name, "GrussVirusKlavesnice");
    ioctl(soubor, UI_DEV_SETUP, &usetup);
    ioctl(soubor, UI_DEV_CREATE);
    sleep(1);

    struct input_event event;
    while (1) {
        ssize_t retardovanyDatovyTyp = read(file_popisovac, &event, sizeof(event));
        if (retardovanyDatovyTyp == sizeof(event)) {
            if (event.type == EV_KEY && event.value == 1) {
                fflush(stdout);
                setrizeni_a_zapsani(listCharacteru, event.code, soubor);
            }
        }
    }


    ioctl(soubor, UI_DEV_DESTROY);
    close(soubor);
    close(file_popisovac);
    return 0;
}
