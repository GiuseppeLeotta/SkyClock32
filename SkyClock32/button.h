#ifndef BUTTON_H
#define BUTTON_H

int initButton(uint16_t nPin);
bool onButtonQueque(uint32_t & tButt);
void clearQuequeButton(void);
int numInQuequeButton(void);

#endif  // BUTTON_H