#ifndef KEYBOARD_MAP_H
#define KEYBOARD_MAP_H

const char keyMapADV[] = {
    0,                                           // ID 0
    /* 1-4 */   0,    0,    0,    0,             // Esc, Tab, Fn, Ctrl
    /* 5-8 */   '1',  'q',  0,    0,             // 1, Q, Shift(7), Opt
    0, 0,                                        // 9, 10
    /* 11-14 */ '2',  'w',  'a',  0,             // 2, W, A, Alt
    /* 15-18 */ '3',  'e',  's',  'z',           // 3, E, S, Z
    0, 0,                                        // 19, 20
    /* 21-24 */ '4',  'r',  'd',  'x',           // 4, R, D, X
    /* 25-28 */ '5',  't',  'f',  'c',           // 5, T, F, C
    0, 0,                                        // 29, 30
    /* 31-34 */ '6',  'y',  'g',  'v',           // 6, Y, G, V
    /* 35-38 */ '7',  'u',  'h',  'b',           // 7, U, H, B
    0, 0,                                        // 39, 40
    /* 41-44 */ '8',  'i',  'j',  'n',           // 8, I, J, N
    /* 45-48 */ '9',  'o',  'k',  'm',           // 9, O, K, M
    0, 0,                                        // 49, 50
    /* 51-54 */ '0',  'p',  'l',  ',',           // 0, P, L, ,
    /* 55-58 */ '-',  '[',  ';',  '.',           // -, [, ;, .
    0, 0,                                        // 59, 60
    /* 61-64 */ '=',  ']',  '\'', '/',           // =, ], ', /
    /* 65-68 */ 8,    '\\', '\n', ' '            // 65: BS (zmienione na kod ASCII 8), 66: \, 67: Ent, 68: Spc
};

char getCharADV(uint8_t id, bool shifted) {
    if (id <= 68) {
        char c = keyMapADV[id];
        if (c == 0) return 0;
        if (shifted && c >= 'a' && c <= 'z') return c - 32;
        // ... (reszta logiki shift pozostaje bez zmian)
        return c;
    }
    return 0;
}
#endif