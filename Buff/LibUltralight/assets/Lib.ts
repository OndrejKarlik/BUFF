/// <reference path="./LibUltralight.d.ts" />
import * as LibMouse from "./LibMouse.js";
export * from "./LibMouse.js";

// ===========================================================================================================
// Basics
// ===========================================================================================================

const BUFF_LIBRARY_NAME = "BUFF";

export function createRandomId(length = 8): string {
    const characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    let result = "";
    for (let i = 0; i < length; i++) {
        result += characters.charAt(Math.random() * characters.length);
    }
    console.log(result);
    return result;
}

export function lerp(x: number, y: number, fractionY: number): number {
    return x * (1 - fractionY) + y * fractionY;
}

export function getUltralightVersion(): number | undefined {
    // This is actually necessary to prevent "ReferenceError: ULTRALIGHT_VERSION_IMPL is not defined"
    if ("ULTRALIGHT_VERSION_IMPL" in window) {
        return ULTRALIGHT_VERSION_IMPL;
    } else {
        return undefined;
    }
}

export function isDebug(): boolean {
    if ("IS_DEBUG_IMPL" in window) {
        return IS_DEBUG_IMPL;
    } else {
        return true;
    }
}

export function isWebkit(): boolean {
    // This is the first thing from here that did work. Everything else gives a false positive in Chrome.
    // https://stackoverflow.com/questions/18625321/detect-webkit-browser-in-javascript
    return "webkitConvertPointFromNodeToPage" in window;
}

export function last<T>(container: T[]): T {
    return container[container.length - 1];
}

export function simpleStringHash(input: string): number {
    let hash = 0;
    for (let i = 0; i < input.length; i++) {
        const char = input.charCodeAt(i);
        hash = (hash << 5) - hash + char;
    }
    return hash;
}

const PAGE_LOAD_TIME = new Date();

export function getCurrentTimeString(): string {
    const date = new Date();
    const hours = String(date.getHours()).padStart(2, "0");
    const minutes = String(date.getMinutes()).padStart(2, "0");
    const seconds = String(date.getSeconds()).padStart(2, "0");
    const milliseconds = String(date.getSeconds()).padStart(3, "0");
    return `${hours}:${minutes}:${seconds}.${milliseconds}`;
}

// TODO: Unusable: it overrides source location of the log
// eslint-disable-next-line @typescript-eslint/no-unused-vars
function debugLog(msg: string): void {
    const date: Date = new Date();
    const diff: number = date.getTime() - PAGE_LOAD_TIME.getTime();
    const totalSeconds = Math.floor(diff / 1000);
    const milliseconds = String(diff % 1000).padStart(3, "0");
    const color = simpleStringHash(String(totalSeconds * 12487 + 11903)); // random prime
    const NUM_COLORS = 5;
    const hue = (Math.floor(color % NUM_COLORS) * 360) / NUM_COLORS;
    console.log(
        `%c[${String(totalSeconds).padStart(4, "0")}.${milliseconds}]%c ${msg}`,
        `color: hsl(${hue}, 100%, 40%)`,
        "color: black",
    );
}

function getRandomLoggingColor(): string {
    const NUM_COLORS = 5;
    const hue = (Math.floor(Math.random() * NUM_COLORS) * 360) / NUM_COLORS;
    return `hsl(${hue}, 100%, 40%)`;
}

export function runAsync(func: () => void): void {
    // TODO: Would it be better to use promises?
    setTimeout(func, 0);
}

export function assert(condition: boolean, ...loggingArgs: unknown[]): asserts condition {
    if (!condition) {
        const errorForCallStack = new Error();
        let description = `${BUFF_LIBRARY_NAME} Assert failed!\nTime: ${getCurrentTimeString()}`;
        if (loggingArgs.length > 0) {
            description += `\nAdditional info: ${loggingArgs.toString()}`;
        }
        description += `\nCall Stack: ${errorForCallStack.stack}`;
        const error = new Error(description);
        throw error;
    }
}

export function assertNonNull<T>(value: T | null | undefined): T {
    assert(value !== null && value !== undefined, JSON.stringify(value));
    return value;
}

export function contains<T>(collection: T[], item: T): boolean {
    for (const i of collection) {
        if (i === item) {
            return true;
        }
    }
    return false;
}

// ===========================================================================================================
// Classes
// ===========================================================================================================

// Can use both integers and floats
export class Vector2 {
    x: number;
    y: number;

    constructor(x: number, y: number) {
        this.x = x;
        this.y = y;
    }

    isInteger(): boolean {
        return this.x === Math.floor(this.x) && this.y === Math.floor(this.y);
    }

    toString(): string {
        return `Pixel(${this.x}, ${this.y})`;
    }

    minElement(): number {
        return Math.min(this.x, this.y);
    }
    maxElement(): number {
        return Math.max(this.x, this.y);
    }
    floor(): Vector2 {
        return new Vector2(Math.floor(this.x), Math.floor(this.y));
    }
}

export class Rectangle {
    x: number;
    y: number;
    sizeX: number;
    sizeY: number;

    constructor(x: number, y: number, sizeX: number, sizeY: number);
    constructor(x: DOMRect);
    constructor(x: DOMRect | number, y?: number, sizeX?: number, sizeY?: number) {
        if (typeof x === "number") {
            this.x = x;
            // Typescript is only looking at the implementation signature, we must tell it that y and others
            // cannot be undefined here
            this.y = y!;
            this.sizeX = sizeX!;
            this.sizeY = sizeY!;
        } else {
            this.x = x.x;
            this.y = x.y;
            this.sizeX = x.width;
            this.sizeY = x.height;
        }
    }

    maxX(): number {
        return this.x + this.sizeX;
    }
    maxY(): number {
        return this.y + this.sizeY;
    }

    center(): Vector2 {
        return new Vector2(this.x + this.sizeX / 2, this.y + this.sizeY / 2);
    }

    toString(): string {
        return `Rectangle(x=${this.x}, y=${this.y}, width=${this.sizeX}, height=${this.sizeY})`;
    }
}

export class Array2D<T> {
    readonly #size: Vector2;
    #data: T[];

    constructor(width: number, height: number, initial?: T) {
        this.#size = new Vector2(width, height);
        assert(
            this.#size.isInteger(),
            `Trying to construct pixel with non-integer values!\nx=${this.#size.x}, y=${this.#size.y}`,
        );
        this.#data = Array<T>(width * height);

        if (initial !== undefined) {
            for (let i = 0; i < width * height; ++i) {
                this.#data[i] = initial;
            }
        }
    }

    getSize(): Vector2 {
        return this.#size;
    }
    getPixelCount(): number {
        return this.#size.x * this.#size.y;
    }

    getNthPixel(n: number): T {
        assert(n >= 0 && n < this.getPixelCount());
        return this.#data[n];
    }
    setNthPixel(n: number, value: T): void {
        assert(n >= 0 && n < this.getPixelCount());
        this.#data[n] = value;
    }

    fill(value: T): void {
        for (let i = 0; i < this.getPixelCount(); ++i) {
            this.setNthPixel(i, value);
        }
    }
    get(pixel: Vector2): T {
        return this.#data[this.#map(pixel)];
    }
    set(pixel: Vector2, value: T): void {
        this.#data[this.#map(pixel)] = value;
    }

    #map(pixel: Vector2): number {
        assert(
            pixel.isInteger(),
            `Trying to access pixel with non-integer values!\nx=${pixel.x}, y=${pixel.y}`,
        );
        assert(
            pixel.x >= 0 && pixel.x < this.#size.x,
            `Array2D index out of bounds: x = ${pixel.x}, width = ${this.#size.x}`,
        );
        assert(
            pixel.y >= 0 && pixel.y < this.#size.y,
            `Array2D index out of bounds: y = ${pixel.y}, height = ${this.#size.y}`,
        );
        return pixel.x + pixel.y * this.#size.x;
    }
}

// ===========================================================================================================
// Misc advanced
// ===========================================================================================================

export function enlargeRect(rect: Rectangle, amountPx: number): Rectangle {
    return new Rectangle(
        rect.x - amountPx,
        rect.y - amountPx,
        rect.sizeX + 2 * amountPx,
        rect.sizeY + 2 * amountPx,
    );
}

export function intersectsRect(rect: Rectangle | DOMRect, point: Vector2): boolean {
    if (rect instanceof DOMRect) {
        rect = new Rectangle(rect);
    }
    return rect.x <= point.x && rect.maxX() >= point.x && rect.y <= point.y && rect.maxY() >= point.y;
}

// Calls callback with 0-1 interpolated values over given time segment
export function animateInterpolate01(
    lengthMs: number,
    callback: (progress: number) => void,
    callbackEnd: (() => void) | null = null,
): void {
    const USE_DEBUG_PRINTING = false;
    const USE_REQUEST_ANIMATION_FRAME = true; // Only for debugging purposes, to remove eventually.
    const startAnimation = function (fn: (timestamp: number) => void): number {
        if (USE_REQUEST_ANIMATION_FRAME) {
            return window.requestAnimationFrame(fn);
        } else {
            return window.setInterval(() => {
                fn(Date.now());
            }, 4);
        }
    };

    let start: number | null = null;
    let debugNumFrames = 0;
    let intervalId = -1;
    const animationFn = function (timeStamp: number): void {
        if (start === null) {
            start = timeStamp;
        }
        let progress = (timeStamp - start) / lengthMs;
        progress = Math.min(progress, 1);
        callback(progress);
        ++debugNumFrames;
        if (progress < 1) {
            if (USE_REQUEST_ANIMATION_FRAME) {
                startAnimation(animationFn);
            }
        } else {
            if (!USE_REQUEST_ANIMATION_FRAME) {
                clearInterval(intervalId);
            }
            if (callbackEnd) {
                callbackEnd();
            }
            if (USE_DEBUG_PRINTING) {
                const methodName = USE_REQUEST_ANIMATION_FRAME ? "requestAnimationFrame" : "setInterval";
                console.log(
                    `Animation (${methodName}) achieved FPS: ${(debugNumFrames * 1000.0) / lengthMs} (${
                        debugNumFrames
                    } frames over ${Math.floor(timeStamp - start)} ms)`,
                );
            }
        }
    };
    intervalId = startAnimation(animationFn);
}

let gDebugTrackDraggingCalls = 0;

/// \param startCallback
/// Called once when the drag is initiated. If false is returned, the drag is not started
///
/// \param dragCallback
/// Params are since last callback. When false is returned, the drag is ended (and endCallback is called)
///
/// \param endCallback
/// called any time the drag is ended. dragCallback has been already called for the latest location
///
/// \param name
/// Only for debugging purposes
///
/// Returns a function that stops tracking if called
export function trackDragging(
    element: HTMLElement,
    startCallback?: (e: MouseEvent) => boolean,
    dragCallback?: (e: MouseEvent, diffX: number, diffY: number) => boolean,
    endCallback?: (e: MouseEvent) => void,
    name?: string,
): () => void {
    const debugIndex = gDebugTrackDraggingCalls++;
    const LOGGING_PREFIX = [
        "%c[trackDragging %d %s]",
        `color: ${getRandomLoggingColor()};background:#EEE; font-weight: bold; padding: 5px;`,
        debugIndex,
        name ?? "",
    ];
    const USE_LOGGING = false;
    enum State {
        OFF,
        BEFORE_FIRST_DRAG,
        ON,
    }
    const dragging = {
        state: State.OFF,
        lastX: -1,
        lastY: -1,
        name,
    };
    function move(e: MouseEvent): void {
        // console.log(...LOGGING_PREFIX, "DRAG("+name+"): mousemove: " + getMouseButtons(e));
        if (dragging.state === State.BEFORE_FIRST_DRAG) {
            if (startCallback) {
                if (!startCallback(e)) {
                    // If startCallback returns false, the drag is not started
                    if (USE_LOGGING) {
                        console.log(
                            ...LOGGING_PREFIX,
                            `DRAG(${name}): Ending drag - startCallback returned false`,
                        );
                    }
                    endImpl(e);
                    return;
                }
            }
            dragging.state = State.ON;
        }
        if (dragging.state === State.ON) {
            if (dragCallback) {
                if (!dragCallback(e, e.clientX - dragging.lastX, e.clientY - dragging.lastY)) {
                    if (USE_LOGGING) {
                        console.log(
                            ...LOGGING_PREFIX,
                            `DRAG(${name}): Ending drag - dragCallback returned false`,
                        );
                    }
                    endImpl(e);
                }
            }
            dragging.lastX = e.clientX;
            dragging.lastY = e.clientY;
        }

        // When jerking the mouse, the mouseup event is sometimes missed. No idea why... so we need to fix it
        if (!LibMouse.isMouseButtonDown(e, LibMouse.MouseButton.LEFT)) {
            if (USE_LOGGING) {
                console.log(
                    ...LOGGING_PREFIX,
                    `DRAG(${name}): Ending drag - mouse released (within move event)`,
                );
            }
            endImpl(e);
        }
    }
    function up(e: MouseEvent): void {
        if (USE_LOGGING) {
            console.log(
                ...LOGGING_PREFIX,
                `DRAG(${name}): mouseup: ${LibMouse.getMouseButtons(e)}, dragging.state = ${
                    dragging.state
                }, mouse = ${LibMouse.getMouseButtons(e)}`,
            );
        }
        // TODO: We cannot check that specifically LMB was released, beacuse when LMB is released chrome
        // fires this even with LMB already released and Safari with it still down.
        // if (dragging.state != State.OFF /*&& !isMouseButtonDown(e, LibMouse.MouseButton.LEFT)*/) {
        assert(dragging.state !== State.OFF);
        // console.log(...LOGGING_PREFIX, `DRAG(${name}): Ending drag - mouse released (up event)`);
        endImpl(e);
        // }
    }
    function endImpl(e: MouseEvent): void {
        assert(dragging.state !== State.OFF);
        if (dragging.state === State.ON && endCallback) {
            endCallback(e);
        }
        dragging.state = State.OFF;
        if (USE_LOGGING) {
            console.log(...LOGGING_PREFIX, `DRAG(${name}): Removing event listeners`);
        }
        document.removeEventListener("mousemove", move);
        document.removeEventListener("mouseup", up);
    }
    function down(e: MouseEvent): void {
        if (LibMouse.getEventOriginButton(e) === LibMouse.MouseButton.LEFT) {
            // e.stopPropagation();
            if (USE_LOGGING) {
                console.log(...LOGGING_PREFIX, `DRAG(${name}): Starting dragging:`);
            }
            assert(dragging.state === State.OFF, "Attempting new drag while previous is still active");
            dragging.state = State.BEFORE_FIRST_DRAG;
            dragging.lastX = e.clientX;
            dragging.lastY = e.clientY;
            document.addEventListener("mousemove", move);
            document.addEventListener("mouseup", up);
            e.stopPropagation();
        }
    }
    element.addEventListener("mousedown", down);
    if (USE_LOGGING) {
        console.log(
            ...LOGGING_PREFIX,
            `Starting trackDragging on ${name}\nPrevious dragging here: ${element.dataset.trackDraggingDebug}`,
        );
    }
    const debuggingEntry = `${debugIndex} ${name}\n`;
    element.dataset.trackDraggingDebug = `${debuggingEntry}${element.dataset.trackDraggingDebug ?? ""}`;
    return function () {
        if (USE_LOGGING) {
            console.log(...LOGGING_PREFIX, `Stopping trackDragging on ${name}`);
        }
        element.dataset.trackDraggingDebug = element.dataset.trackDraggingDebug!.replace(debuggingEntry, "");
        element.removeEventListener("mousedown", down);
    };
}

export function makeDraggable(
    element: HTMLElement,
    startCallback?: (e: MouseEvent) => boolean,
    dragCallback?: (e: MouseEvent, diffX: number, diffY: number) => boolean,
    endCallback?: (e: MouseEvent) => void,
): void {
    let draggingPos = [0, 0];
    trackDragging(
        element,
        (e) => {
            const position = window.getComputedStyle(element).position;
            if (position === "absolute") {
                draggingPos = [element.offsetLeft, element.offsetTop];
            } else {
                draggingPos = [convertToPx(element.style.left), convertToPx(element.style.top)];
                assert(position === "relative", "Element must be positioned absolutely or relatively.");
            }
            element.classList.add("dragging");
            if (startCallback) {
                return startCallback(e);
            } else {
                return true;
            }
        },
        (e, dX, dY) => {
            // console.log("offsetLeft: " + element.offsetLeft);
            // console.log("dragging: " + dX + ", " + dY);
            draggingPos[0] += dX;
            draggingPos[1] += dY;
            element.style.left = `${draggingPos[0]}px`;
            element.style.top = `${draggingPos[1]}px`;
            if (dragCallback) {
                return dragCallback(e, dX, dY);
            } else {
                return true;
            }
        },
        (e) => {
            element.classList.remove("dragging");
            if (endCallback) {
                endCallback(e);
            }
        },
        "makeDraggable",
    );
}

// ===========================================================================================================
// Typescript
// ===========================================================================================================

// eslint-disable-next-line @typescript-eslint/no-unused-vars
export function safeCast<TTo>(object: object | null, type: TTo | null = null): TTo {
    assert(object !== null && object !== undefined);
    // if (object.constructor.name == target.constructor.name) {
    return object as TTo;
    // } else {
    //    throw new Error("safeCast failed");
    // }
}

// type Constructor<T> = new (...args: any[]) => T;

// export function ofType<TElements, TFilter extends TElements>(array: TElements[],
//                                                              filterType: Constructor<TFilter>): TFilter[] {
//     return <TFilter[]>array.filter(e => e instanceof filterType);
// }

export function toStr(input: number): string {
    return `${input}`;
}
export function toInt(input: string): number {
    const regExp = (function () {
        const cached = /^[+-]?[0-9]+$/u;
        return (): RegExp => cached;
    })();
    assert(regExp().test(input), input);
    return parseInt(input, 10);
}
// ===========================================================================================================
// CSS
// ===========================================================================================================

/// Returns numerical value from CSS string
export function convertToVw(cssString: string): number {
    // console.log("convertToVw " + cssString);
    const number = parseInt(cssString, 10); // There might be a suffix, so not using toInt
    if (cssString.endsWith("px")) {
        return (number * 100) / document.body.clientWidth;
    } else if (cssString.endsWith("vw")) {
        return number;
    } else {
        throw new Error(`Unknown unit: '${cssString}'!`);
    }
}

/// Returns numerical value from CSS string
export function convertToPx(cssString: string): number {
    // console.log("convertToPx: " + cssString);
    const number = parseInt(cssString, 10); // There might be a suffix, so not using toInt
    if (cssString.endsWith("px")) {
        return number;
    } else if (cssString.endsWith("vw")) {
        return (number * document.body.clientWidth) / 100;
    } else {
        throw new Error(`Unknown unit: '${cssString}'!`);
    }
}

export function overrideCss(css: string): void {
    const style = document.createElement("style");
    style.innerHTML = css;
    style.classList.add("overrideCss");
    document.head.appendChild(style);
}

export function endOverrideCss(): void {
    const res = document.querySelector("head > style.overrideCss");
    if (res) {
        res.remove();
    }
}

// ===========================================================================================================
// DOM
// ===========================================================================================================

export function createDomElement<T = Element>(html: string): T {
    const result = document.createElement("div");
    result.innerHTML = html;
    assert(result.childElementCount === 1, html);
    return safeCast<T>(result.children[0]);
}

export function querySelectorUnique<T = Element>(
    selector: string,
    domElement: Element | Document = document,
): T {
    const resultList = domElement.querySelectorAll(selector);
    assert(
        resultList.length === 1,
        `querySelectorUnique(${selector}): Expected exactly one element, found: ${resultList.length}`,
    );
    const result = safeCast<T>(resultList.item(0));
    return result;
}

export function isTransitiveChild(child: Element, parent: Element): boolean {
    while (child.parentElement) {
        if (child.parentElement === parent) {
            return true;
        }
        child = child.parentElement;
    }
    return false;
}

export function getTypedElementById<Type extends HTMLElement>(id: string, type: Type | null = null): Type {
    const result = document.getElementById(id);
    if (result) {
        return safeCast<Type>(result, type);
    } else {
        throw new Error(`getTypedElementById: ${id} is null`);
    }
}

// ===========================================================================================================
// DOM sizing, positioning
// ===========================================================================================================

export const enum SizeType {
    CONTENT,
    WITH_PADDING,
    WITH_BORDER,
    WITH_MARGIN,
}

/// Returns the content size of the element, i.e. size of the element without padding, border, margin
export function getElementSize(element: HTMLElement, sizeType: SizeType): Vector2 {
    // From
    // https://stackoverflow.com/questions/25197184/get-the-height-of-an-element-minus-padding-margin-border-widths
    const style = getComputedStyle(element);
    switch (sizeType) {
        case SizeType.CONTENT: {
            const paddingX = parseFloat(style.paddingLeft) + parseFloat(style.paddingRight);
            const paddingY = parseFloat(style.paddingTop) + parseFloat(style.paddingBottom);
            return new Vector2(element.clientWidth - paddingX, element.clientHeight - paddingY);
        }
        case SizeType.WITH_PADDING:
            return new Vector2(element.clientWidth, element.clientHeight);
        case SizeType.WITH_BORDER:
            return new Vector2(element.offsetWidth, element.offsetHeight);
        case SizeType.WITH_MARGIN: {
            const marginX = parseFloat(style.marginLeft) + parseFloat(style.marginRight);
            const marginY = parseFloat(style.marginTop) + parseFloat(style.marginBottom);
            return new Vector2(element.offsetWidth + marginX, element.offsetHeight + marginY);
        }
        default:
            throw new Error("Invalid option at getElementSize");
    }
}

/// \param centerTo Array of x,y coordinates in px where to center the element
/// \param pivotPoint Optional Array of x,y coordinates of what to center in the target element. Default value
/// is 1/2 of the element
export function centerElementAt(element: HTMLElement, centerTo: Vector2, pivotPoint?: Vector2): void {
    assert(
        window.getComputedStyle(element).position === "absolute" ||
            window.getComputedStyle(element).position === "relative",
        `Element must be positioned absolutely or relatively, instead is: ${window.getComputedStyle(element).position}`,
    );
    const current = element.getBoundingClientRect();
    // console.log("CURRENT POSITION: " + current.x + ", size: " + current.width);
    if (pivotPoint === undefined) {
        pivotPoint = new Vector2(current.width / 2, current.height / 2);
    }
    const desired: Vector2 = new Vector2(centerTo.x - pivotPoint.x, centerTo.y - pivotPoint.y);
    // console.log("DESIRED POSITION: " + centerTo);
    // console.log("centerElementAt: element.style.left before: " + element.style.left);
    element.style.left = `${convertToPx(element.style.left) + desired.x - current.left}px`;
    element.style.top = `${convertToPx(element.style.top) + desired.y - current.top}px`;
    // console.log("centerElementAt: element.style.left after: " + element.style.left);
}

export function getElementCenter(element: HTMLElement): Vector2 {
    const rect = element.getBoundingClientRect();
    return new Vector2(rect.left + rect.width / 2, rect.top + rect.height / 2);
}

// ===========================================================================================================
// UI Widgets
// ===========================================================================================================

interface ContextMenuItem {
    name: string;
    action: () => void;
}

export function showContextMenu(e: MouseEvent, items: ContextMenuItem[]): void {
    console.log("Opening context menu...");
    // https://coolors.co/93827f-f3f9d2-bdc4a7-2f2f2f-92b4a7
    const element = createDomElement<HTMLElement>(`
        <div style="
            position: absolute;
            left: ${e.clientX}px;
            top: ${e.clientY}px;
            z-index: 2000000000;
            background-color: #F2F7F2;
            color: #443850;
        ">
        </div>`);
    for (const item of items) {
        const itemElement = createDomElement(
            `<button type="button" style="padding: 4px 8px;">${item.name}</button>`,
        );
        itemElement.addEventListener("click", () => {
            item.action();
            safeCast<HTMLInputElement>(element.children[0]).blur(); // Trigger blur event to close the menu
        });
        element.appendChild(itemElement);
    }
    document.body.insertBefore(element, document.body.firstElementChild);
    safeCast<HTMLInputElement>(element.children[0]).focus();
    element.children[0].addEventListener("blur", (): void => {
        element.remove();
    });
}

export function createDebugButton(name: string, action: () => void): HTMLElement | null {
    if (!isDebug()) {
        return null;
    }
    console.log(`Create debugging button: ${name}`);
    const e = document.createElement("input");
    e.classList.add("debugButton");
    e.type = "button";
    e.value = name;
    e.onclick = action;
    return e;
}

// ===========================================================================================================
// MetaStruct connection
// ===========================================================================================================

interface MetaStructParam {
    value: Record<string, string | boolean | number>;
}

// CALLED FROM C++
export function connectMetaStructParam(id: string): void {
    const element: HTMLInputElement = getTypedElementById<HTMLInputElement>(id);
    // assert(element !== null, `connectMetaStructParam: ID does not exist: ${id}`);
    element.addEventListener("input", () => {
        const toSet: MetaStructParam = { value: {} };
        if (element.type === "checkbox") {
            // Get the value of the checkbox
            toSet.value[id] = element.checked;
        } else if (element.type === "number") {
            toSet.value[id] = toInt(element.value);
        } else {
            toSet.value[id] = element.value;
        }
        console.log(element.type);
        console.log(toSet);
        setMetaStructParam(id, JSON.stringify(toSet));
    });
    updateMetaStructParam(id);
}

// CALLED FROM C++
export function updateMetaStructParam(id: string): boolean {
    const elementIn = document.getElementById(id);
    if (!elementIn) {
        return false;
    }
    const element = safeCast<HTMLInputElement>(elementIn);
    const json = getMetaStructParam(id);

    // TODO: Write a JSON validator for this
    // eslint-disable-next-line @typescript-eslint/no-unsafe-assignment
    const value: MetaStructParam = JSON.parse(json);
    const toSet = value.value[id];
    console.log(toSet);
    if (element.type === "checkbox") {
        assert(
            typeof toSet === "boolean",
            `updateMetaStructParam: ID: ${id} Expected boolean, got: ${toSet}`,
        );
        element.checked = toSet;
    } else if (element.type === "number") {
        assert(typeof toSet === "number", `updateMetaStructParam: ID: ${id} Expected number, got: ${toSet}`);
        element.value = toStr(toSet);
    } else {
        assert(typeof toSet === "string", `updateMetaStructParam: ID: ${id} Expected string, got: ${toSet}`);
        element.value = toSet;
        // assert(element.value == toSet, "element.value = " + element.value + ", toSet = " + toSet);
    }
    return true;
}

// ===========================================================================================================
// MVC
// ===========================================================================================================

export class DomSynchronizedArray<T, TElement extends Element = HTMLElement> {
    readonly #array: T[];
    readonly #listElement: HTMLElement;
    readonly #newFunction: (x: T) => TElement;

    constructor(listElement: HTMLElement, newFunction: (x: T) => TElement) {
        this.#array = [];
        this.#listElement = listElement;
        this.#newFunction = newFunction;
    }

    size(): number {
        this.#checkConsistency();
        return this.#array.length;
    }

    get(index: number): T {
        this.#checkConsistency();
        assert(index >= 0 && index < this.size(), index, this.size());
        return this.#array[index];
    }
    getElement(item: T): TElement {
        this.#checkConsistency();
        const index = this.#array.indexOf(item);
        assert(index !== -1);
        return safeCast<TElement>(this.#listElement.children[index]);
    }

    findIndex(functor: (item: T) => boolean): number {
        this.#checkConsistency();
        return this.#array.findIndex(functor);
    }
    indexOf(item: T): number {
        this.#checkConsistency();
        return this.#array.indexOf(item);
    }

    some(functor: (item: T) => boolean): boolean {
        this.#checkConsistency();
        return this.#array.some(functor);
    }
    contains(item: T): boolean {
        return this.#array.includes(item);
    }

    /// Moves an element from "from" position to "where" position (i.e. inserts it before the current element at where)
    move(from: number, where: number): void {
        assert(from >= 0 && from < this.size(), from, this.size());
        assert(where >= 0 && where <= this.size(), where, this.size());
        assert(from !== where, from);

        this.#array.splice(where, 0, this.#array[from]);
        if (where < from) {
            this.#array.splice(from + 1, 1);
        } else {
            this.#array.splice(from, 1);
        }

        this.#listElement.insertBefore(this.#listElement.children[from], this.#listElement.children[where]);
        this.#checkConsistency();
    }

    iterate(callback: (item: T, element: TElement, index: number) => void): void {
        this.#checkConsistency();
        for (let i = 0; i < this.#array.length; ++i) {
            callback(this.#array[i], safeCast<TElement>(this.#listElement.children[i]), i);
        }
    }

    push(item: T): void {
        this.#array.push(item);
        const newElement = this.#newFunction(item);
        this.#listElement.appendChild(newElement);
        this.#checkConsistency();
    }

    splice(index: number, count: number, ...toInsert: T[]): void {
        assert(index >= 0 && index <= this.size(), index, count, this.size());
        assert(count >= 0 && index + count <= this.size(), index, count, this.size());

        this.#array.splice(index, count, ...toInsert);
        for (let i = 0; i < count; ++i) {
            assert(this.#listElement.children[index] !== undefined);
            this.#listElement.children[index].remove();
        }
        for (let i = 0; i < toInsert.length; ++i) {
            this.#listElement.insertBefore(
                this.#newFunction(toInsert[i]),
                this.#listElement.children[index + i],
            );
        }
        this.#checkConsistency();
    }
    remove(index: number): void {
        this.splice(index, 1);
    }

    #checkConsistency(): void {
        //console.log(this.#array);
        //console.log(this.#listElement.children);
        assert(
            this.#array.length === this.#listElement.children.length,
            "Array and DOM element mismatch",
            this.#array.length,
            this.#listElement.children.length,
        );
    }

    //pop(item: T): void {
    //    const index = this.#array.indexOf(item);
    //    if (index !== -1) {
    //        this.#array.splice(index, 1);
    //        this.#element.children[index].remove();
    //    }
    //}
}
