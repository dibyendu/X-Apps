/*
 * File:   bezier.c
 * Author: dibyendu
 *
 * Created on 4 November, 2010
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

typedef struct {
    unsigned long long x, y;
} point;

const int windowWidth = 1024,
          windowHeight = 768,
          rectWidth = 800,
          rectHeight = 500,
          pointRadius = 4;

const char *text = "#000000",
           *rect = "#00BBFF",
           *pointColour = "#FF0000",
           *curve = "#00FF00",
           *message = "Bezier Curve";

int *
drawText(Display *d, Window *w, GC *gc, int textX, int textY, const char *str) {
    XFontStruct *font;
    char **list;
    int *textWidth_and_Height, returnNo;

    textWidth_and_Height = (int *) calloc(2, sizeof (int));

    list = XListFonts(d, "-*-*-bold-r-normal--*-*-100-100-c-*-*", 200, &returnNo);
    if (returnNo) {
        srand(time(NULL));
        static int fontIndex = -1;
        fontIndex = fontIndex == -1 ? rand() % returnNo : fontIndex;
        font = XLoadQueryFont(d, *(list + fontIndex));
        XFreeFontNames(list);
    }

    if (!font)
        return NULL;

    XSetFont(d, *gc, font->fid);

    textWidth_and_Height[0] = XTextWidth(font, str, strlen(str));
    textWidth_and_Height[1] = font->ascent + font->descent;
    if (textX == 0 && textY == 0) {
        textX = (windowWidth - textWidth_and_Height[0]) / 2;
        textY = ((windowHeight - rectHeight) / 2 - textWidth_and_Height[1]) / 2 + textWidth_and_Height[1] / 2;
        free(textWidth_and_Height);
    } else
        textY -= textWidth_and_Height[1];

    XDrawImageString(d, *w, *gc, textX, textY, str, strlen(str));
    return textWidth_and_Height;
}

void
createGC(Display *d, Window *w, int screen, int lineWidth, GC *gc, const char *colour, XColor *xcolor) {
    *gc = XCreateGC(d, *w, 0, 0);
    XParseColor(d, DefaultColormap(d, screen), colour, xcolor);
    XAllocColor(d, DefaultColormap(d, screen), xcolor);
    XSetForeground(d, *gc, xcolor->pixel);
    XSetLineAttributes(d, *gc, lineWidth, LineSolid, CapRound, JoinRound);
    XSetFillStyle(d, *gc, FillSolid);
    return;
}

bool
checkPointLocation(int x, int y) {
    int rectX = (windowWidth - rectWidth) / 2, rectY = (windowHeight - rectHeight) / 2;
    if (x - pointRadius <= rectX || x + pointRadius >= rectX + rectWidth) return false;
    if (y - pointRadius <= rectY || y + pointRadius >= rectY + rectHeight) return false;
    return true;
}

long long
nCr(int n, int r) {
    if (n < r) return -1;
    int i, j;
    long long productN = 1, productR = 1;
    for (i = n, j = 1; j <= r; i--, j++) {
        productN *= i;
        productR *= j;
    }
    return productN / productR;
}

int main() {
    Display *d;
    Window w, subw;
    GC rectGc, textGc, pointGc, curveGc, invGc;
    XColor rectColor, pointColor, curveColor, textColor;
    XEvent e;
    KeySym key;

    point *p = NULL;

    int rectX = (windowWidth - rectWidth) / 2, rectY = (windowHeight - rectHeight) / 2, s, noOfPoints = 0;
    int exposeCount = 0;
    char buffer[3];

    d = XOpenDisplay(NULL);
    s = DefaultScreen(d);
    w = XCreateSimpleWindow(d, RootWindow(d, s), 0, 0, windowWidth, windowHeight, 0, 0, WhitePixel(d, s));

    XStoreName(d, w, "Bezier Curve Window");
    XSelectInput(d, w, ExposureMask | KeyPressMask | ButtonPressMask);
    XMapWindow(d, w);
    XMoveWindow(d, w, (DisplayWidth(d, s) - windowWidth) / 2, (DisplayHeight(d, s) - windowHeight) / 2);

    textGc = XCreateGC(d, w, 0, 0);
    XParseColor(d, DefaultColormap(d, s), text, &textColor);
    XAllocColor(d, DefaultColormap(d, s), &textColor);
    XSetForeground(d, textGc, textColor.pixel);
    XSetBackground(d, textGc, WhitePixel(d, s));
    XSetLineAttributes(d, textGc, 1, LineOnOffDash, CapRound, JoinRound);

    createGC(d, &w, s, 2, &rectGc, rect, &rectColor);
    createGC(d, &w, s, 2, &pointGc, pointColour, &pointColor);
    createGC(d, &w, s, 1, &curveGc, curve, &curveColor);

    invGc = XCreateGC(d, w, 0, 0);
    XSetForeground(d, invGc, WhitePixel(d, s));
    XSetBackground(d, invGc, WhitePixel(d, s));
    XSetFillStyle(d, invGc, FillSolid);

    while (true) {
        XNextEvent(d, &e);
        switch (e.type) {
            case Expose:
                exposeCount++;
                drawText(d, &w, &textGc, 0, 0, message);
                XDrawRectangle(d, w, rectGc, rectX, rectY, rectWidth, rectHeight);
                if (exposeCount == 1)
                    drawText(d, &w, &textGc, rectX + rectWidth / 2 - 180, rectY + rectHeight / 2, "Press <h> for help");
                break;
            case KeyPress:
                key = XLookupKeysym(&e.xkey, 0);
                if (key == XK_c) {
                    if (p) {
                        free(p);
                        p = NULL;
                        noOfPoints = 0;
                    }
                    XClearWindow(d, w);
                    drawText(d, &w, &textGc, 0, 0, message);
                    XDrawRectangle(d, w, rectGc, rectX, rectY, rectWidth, rectHeight);
                } else if (key == XK_h) {
                    XClearWindow(d, w);
                    subw = XCreateSimpleWindow(d, w, 0, 0, windowWidth, windowHeight, 0, 0, WhitePixel(d, s));
                    XSelectInput(d, subw, ExposureMask | KeyPressMask);
                    XMapWindow(d, subw);
                    while (true) {
                        XEvent sube;
                        XNextEvent(d, &sube);
                        switch (sube.type) {
                            case Expose:
                                drawText(d, &subw, &textGc, rectX - 80, rectY + 40,
                                        "* press <left mouse button> inside the rectangle");
                                drawText(d, &subw, &textGc, rectX - 80, rectY + 80,
                                        "  to generate points");
                                drawText(d, &subw, &textGc, rectX - 80, rectY + 160,
                                        "* press <right mouse button> to draw bezier curve");
                                drawText(d, &subw, &textGc, rectX - 80, rectY + 240,
                                        "* press <c> to erase all");
                                drawText(d, &subw, &textGc, rectX - 80, rectY + 320,
                                        "* press <h> for help");
                                drawText(d, &subw, &textGc, rectX - 80, rectY + 400,
                                        "* press <any key> to exit from this screen");
                                break;
                            case KeyPress:
                                XDestroyWindow(d, subw);
                                goto end;
                        }
                    }
end:
                    ;
                    drawText(d, &w, &textGc, 0, 0, message);
                    XDrawRectangle(d, w, rectGc, rectX, rectY, rectWidth, rectHeight);
                } else exit(0);
                break;
            case ButtonPress:
                if (e.xbutton.button == Button1) {
                    if (exposeCount == 1) {
                        XClearWindow(d, w);
                        drawText(d, &w, &textGc, 0, 0, message);
                        XDrawRectangle(d, w, rectGc, rectX, rectY, rectWidth, rectHeight);
                        exposeCount++;
                    }
                    if (checkPointLocation(e.xbutton.x, e.xbutton.y)) {
                        noOfPoints++;
                        p = (point *) realloc(p, sizeof (point) * noOfPoints);
                        (p + noOfPoints - 1)->x = e.xbutton.x;
                        (p + noOfPoints - 1)->y = e.xbutton.y;
                        XFillArc(d, w, pointGc, e.xbutton.x - pointRadius, e.xbutton.y - pointRadius,
                                pointRadius * 2, pointRadius * 2, 0, 360 * 64);
                        sprintf(buffer, "%d", noOfPoints);
                        drawText(d, &w, &textGc, e.xbutton.x, e.xbutton.y, buffer);
                        if (noOfPoints > 1)
                            XDrawLine(d, w, textGc, p[noOfPoints - 2].x, p[noOfPoints - 2].y, p[noOfPoints - 1].x, p[noOfPoints - 1].y);
                    }
                }
                if (e.xbutton.button == Button3) {
                    if (p) {
                        int i, j = 1, temp = 0, *textWidth_and_Height[3];
                        point bezierPrev = *p, bezierNext;
                        double t;
                        for (i = 0; i < noOfPoints; i++) {
                            sprintf(buffer, "%d", i + 1);
                            drawText(d, &w, &invGc, (p + i)->x, (p + i)->y, buffer);
                            XFillArc(d, w, invGc, (p + i)->x - pointRadius, (p + i)->y - pointRadius,
                                    pointRadius * 2, pointRadius * 2, 0, 360 * 64);
                            XFillArc(d, w, pointGc, (p + i)->x - pointRadius / 2, (p + i)->y - pointRadius / 2,
                                    2 * pointRadius / 2, 2 * pointRadius / 2, 0, 360 * 64);
                        }
                        for (i = 0; i < noOfPoints - 1; i++)
                            XDrawLine(d, w, textGc, p[i].x, p[i].y, p[i + 1].x, p[i + 1].y);
                        drawText(d, &w, &invGc, rectX + rectWidth / 4, rectY + rectHeight + (windowHeight - rectHeight) / 4,
                            "Done   % [                                          ]");
                        textWidth_and_Height[0] = drawText(d, &w, &textGc, rectX + rectWidth / 4, rectY + rectHeight + (windowHeight - rectHeight) / 4,
                            "Done ");
                        textWidth_and_Height[1] = drawText(d, &w, &textGc, rectX + rectWidth / 4 + *textWidth_and_Height[0], rectY + rectHeight + (windowHeight - rectHeight) / 4,
                            "  % [");
                        drawText(d, &w, &textGc, rectX + rectWidth / 4 + *textWidth_and_Height[0]+ *textWidth_and_Height[1],
                                rectY + rectHeight + (windowHeight - rectHeight) / 4,
                                "                                          ]");
                        for (t = 0; t <= 1; t += 0.0000008) {
                            bezierNext.x = bezierNext.y = 0;
                            for (i = 0; i < noOfPoints; i++) {
                                bezierNext.x += nCr(noOfPoints - 1, i) * pow(1 - t, noOfPoints - 1 - i) * pow(t, i) * ((p + i)->x);
                                bezierNext.y += nCr(noOfPoints - 1, i) * pow(1 - t, noOfPoints - 1 - i) * pow(t, i) * ((p + i)->y);
                            }
                            XDrawLine(d, w, curveGc, bezierPrev.x, bezierPrev.y, bezierNext.x, bezierNext.y);
                            i = t * 100;
                            if (i == j) {
                                j++;
                                sprintf(buffer, "%2d", i);
                                drawText(d, &w, &invGc, rectX + rectWidth / 4 + *textWidth_and_Height[0],
                                        rectY + rectHeight + (windowHeight - rectHeight) / 4, buffer);
                                drawText(d, &w, &textGc, rectX + rectWidth / 4 + *textWidth_and_Height[0],
                                        rectY + rectHeight + (windowHeight - rectHeight) / 4, buffer);
                                if (i % 5 == 1) {
                                    textWidth_and_Height[2] = drawText(d, &w, &textGc, rectX + rectWidth / 4 + *textWidth_and_Height[0]+ *textWidth_and_Height[1] + temp,
                                            rectY + rectHeight + (windowHeight - rectHeight) / 4, "==");
                                    temp += *textWidth_and_Height[2];
                                }
                            }
                            bezierPrev.x = bezierNext.x;
                            bezierPrev.y = bezierNext.y;
                        }
                        drawText(d, &w, &invGc, rectX + rectWidth / 4 + *textWidth_and_Height[0],
                                rectY + rectHeight + (windowHeight - rectHeight) / 4, buffer);
                        drawText(d, &w, &textGc, rectX + rectWidth / 4 + *textWidth_and_Height[0],
                                rectY + rectHeight + (windowHeight - rectHeight) / 4, "100%");
                        drawText(d, &w, &textGc, rectX + rectWidth / 4 + *textWidth_and_Height[0]+ *textWidth_and_Height[1] + temp,
                                rectY + rectHeight + (windowHeight - rectHeight) / 4, "==");
                        free(p);
                        p = NULL;
                        noOfPoints = 0;
                    }
                }
        }
    }
    XDestroyWindow(d, w);
    XCloseDisplay(d);
    return (EXIT_SUCCESS);
}
