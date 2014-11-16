/* 
 * File:   planet.c
 * Author: dibyendu
 *
 * Created on 17 October, 2010
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <complex.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#define NO_OF_PLANETS 8

const double earthRadius = 6,
             earthOrbitSemiMajor = 60,
             earthOrbitSemiMinor = 40,
             earthAngularVelocity = 2;
/*
 * Mercury  Venus  Earth  Mars  Jupiter  Saturn  Uranus  Neptune
 */
const double planetData[] = {0.6, 0.9, 1, 0.7, 5.4, 4.4, 2.6, 1.6,
    2, 1.5, 1, 0.8, 0.72, 0.65, 0.58, 0.51,
    0.6, 0.8, 1, 1.5, 3, 4.4, 6, 8,
    0.4, 0.6, 0.8, 1.3, 2.8, 4.2, 5.8, 7.8,
    0.2056, 0.0068, 0.0167, 0.0934, 0.0483, 0.0560, 0.0461, 0.0097};

const int windowWidth = 1024,
          windowHeight = 768,
          rectWidth = 1000,
          rectHeight = 660,
          sunRadius = 10;

const char *rect = "#00BBFF",
           *sun = "#FF0000",
           *text = "#000000",
           *planetColour[NO_OF_PLANETS] = {"#0000FF", "#229942", "#3594BB", "#A52828", "#C14B4B",
				           "#62BFED", "#4CBA8B", "#AD6E4A"};
const char *message = "Planetary Motion Simulator";

typedef double complex Point;

typedef struct {
    double radius, angVelocity, orbitSemiMajor, orbitSemiMinor, orbitEccentricity;
} planet;

planet p[8];

Point
pointOnEllipse(double semiMajor, double semiMinor, int angleInDegrees, Point centre) {
    return centre + (creal(semiMajor * cexp(-M_PI * I * angleInDegrees / 180.0)) + I * cimag(semiMinor * cexp(-M_PI * I * angleInDegrees / 180.0)));
}

void
setPlanetAttributes(planet *p) {
    for (int i = 0; i < NO_OF_PLANETS; i++) {
        p[i].radius = planetData[i] * earthRadius;
        p[i].angVelocity = planetData[i + 8] * earthAngularVelocity;
        p[i].orbitSemiMajor = planetData[i + 16] * earthOrbitSemiMajor;
        p[i].orbitSemiMinor = planetData[i + 24] * earthOrbitSemiMinor;
        p[i].orbitEccentricity = planetData[i + 32];
    }
    return;
}

int
drawText(Display *d, int screen, Window *w, GC *gc, const char *str) {
    XFontStruct *font;
    XColor textColor;
    char **list;
    int textWidth, textHeight, textX, textY, returnNo;

    list = XListFonts(d, "-*-*-bold-r-normal--*-*-100-100-c-*-*", 200, &returnNo);
    if (returnNo) {
        srand(time(NULL));
        static int fontIndex = rand() % returnNo;
        font = XLoadQueryFont(d, *(list + fontIndex));
        XFreeFontNames(list);
    }

    if (!font)
        return -1;

    XParseColor(d, DefaultColormap(d, screen), text, &textColor);
    XAllocColor(d, DefaultColormap(d, screen), &textColor);
    XSetForeground(d, *gc, textColor.pixel);
    XSetBackground(d, *gc, WhitePixel(d, screen));
    XSetFont(d, *gc, font->fid);

    textWidth = XTextWidth(font, str, strlen(str));
    textHeight = font->ascent + font->descent;
    textX = (windowWidth - textWidth) / 2;
    textY = ((windowHeight - rectHeight) / 2 - textHeight) / 2 + textHeight / 2;

    XDrawImageString(d, *w, *gc, textX, textY, str, strlen(str));
    return 0;
}

void
createGC(Display *d, Window *w, int screen, int lineWidth, GC *gc, const char *str, XColor *color) {
    *gc = XCreateGC(d, *w, 0, 0);
    XParseColor(d, DefaultColormap(d, screen), str, color);
    XAllocColor(d, DefaultColormap(d, screen), color);
    XSetForeground(d, *gc, color->pixel);
    XSetLineAttributes(d, *gc, lineWidth, LineSolid, CapRound, JoinRound);
    XSetFillStyle(d, *gc, FillSolid);
    return;
}

int
main() {
    Display *d;
    Window w;
    GC planetGc[NO_OF_PLANETS], rectGc, textGc, sunGc, invGc;
    XColor sunColor, rectColor, planetColor[NO_OF_PLANETS];
    XEvent e;
    KeySym key;
    int rectX = (windowWidth - rectWidth) / 2, rectY = (windowHeight - rectHeight) / 2;
    Point pt[NO_OF_PLANETS], centre = rectX + (rectWidth / 2) + I * (rectY + (rectHeight / 2));
    int prevAngle[NO_OF_PLANETS];
    int s, sunX = creal(centre) - p[0].orbitSemiMajor * p[0].orbitEccentricity - 10, sunY = cimag(centre), i;

    d = XOpenDisplay(NULL);
    s = DefaultScreen(d);
    w = XCreateSimpleWindow(d, RootWindow(d, s), 0, 0, windowWidth, windowHeight, 0, 0, WhitePixel(d, s));

    setPlanetAttributes(p);

    XStoreName(d, w, "Planetary Motion Simulator Window");
    XSelectInput(d, w, ExposureMask | KeyPressMask);
    XMapWindow(d, w);
    XMoveWindow(d, w, (DisplayWidth(d, s) - windowWidth) / 2, (DisplayHeight(d, s) - windowHeight) / 2);

    textGc = XCreateGC(d, w, 0, 0);

    createGC(d, &w, s, 2, &rectGc, rect, &rectColor);
    createGC(d, &w, s, 1, &sunGc, sun, &sunColor);
    for (i = 0; i < NO_OF_PLANETS; i++)
        createGC(d, &w, s, 1, &planetGc[i], planetColour[i], &planetColor[i]);

    invGc = XCreateGC(d, w, 0, 0);
    XSetForeground(d, invGc, WhitePixel(d, s));
    XSetFillStyle(d, invGc, FillSolid);

    for (i = 0; i < NO_OF_PLANETS; i++)
        prevAngle[i] = 90 * i;

    while (true) {
        XNextEvent(d, &e);
        switch (e.type) {
            case Expose:
                drawText(d, s, &w, &textGc, message);
                XDrawRectangle(d, w, rectGc, rectX, rectY, rectWidth, rectHeight);
                XFillArc(d, w, sunGc, sunX - sunRadius, sunY - sunRadius, sunRadius * 2, sunRadius * 2, 0, 360 * 64);
                for (i = 0; i < NO_OF_PLANETS; i++) {
                    pt[i] = pointOnEllipse(p[i].orbitSemiMajor, p[i].orbitSemiMinor, prevAngle[i], centre);
                    XDrawArc(d, w, planetGc[i], creal(centre) - p[i].orbitSemiMajor, cimag(centre) - p[i].orbitSemiMinor,
                            p[i].orbitSemiMajor * 2, p[i].orbitSemiMinor * 2, 0, 360 * 64);
                    XFillArc(d, w, planetGc[i], creal(pt[i]) - p[i].radius, cimag(pt[i]) - p[i].radius,
                            p[i].radius * 2, p[i].radius * 2, 0, 360 * 64);
                }
                break;
            case KeyPress:
                key = XLookupKeysym(&e.xkey, 0);
                if (key == XK_Return) {
                    drawText(d, s, &w, &textGc, message);
                    for (i = 0; i < NO_OF_PLANETS; i++)
                        XFillArc(d, w, invGc, creal(pt[i]) - p[i].radius, cimag(pt[i]) - p[i].radius,
                            p[i].radius * 2, p[i].radius * 2, 0, 360 * 64);
                    XDrawRectangle(d, w, rectGc, rectX, rectY, rectWidth, rectHeight);
                    XFillArc(d, w, sunGc, sunX - sunRadius, sunY - sunRadius, sunRadius * 2, sunRadius * 2, 0, 360 * 64);
                    for (i = 0; i < NO_OF_PLANETS; i++) {
                        pt[i] = pointOnEllipse(p[i].orbitSemiMajor, p[i].orbitSemiMinor, prevAngle[i], centre);
                        XDrawArc(d, w, planetGc[i], creal(centre) - p[i].orbitSemiMajor, cimag(centre) - p[i].orbitSemiMinor,
                                p[i].orbitSemiMajor * 2, p[i].orbitSemiMinor * 2, 0, 360 * 64);
                        XFillArc(d, w, planetGc[i], creal(pt[i]) - p[i].radius, cimag(pt[i]) - p[i].radius,
                                p[i].radius * 2, p[i].radius * 2, 0, 360 * 64);
                        prevAngle[i] += p[i].angVelocity * (i == 1 || i == 6 ? -1 : 1); // venus & uranus rotate clockwise
                        prevAngle[i] %= 360;
                    }
                } else exit(0);
        }
    }
    XDestroyWindow(d, w);
    XCloseDisplay(d);
    return (EXIT_SUCCESS);
}
