import * as Lib from "./Lib.js";

// ===========================================================================================================
// Mouse utilities
// ===========================================================================================================

/// Returns a bitset:
/// LMB = 1
/// RMB = 2
/// MMB = 4
export function getMouseButtons(e: MouseEvent): number {
    if (Lib.getUltralightVersion() === undefined) {
        return e.buttons;
    } else {
        switch (e.which) {
            // old webkit in ultralight...
            case 1: // LMB
                return 1;
            case 2: // middle
                return 4;
            case 3: // RMB
                return 2;
            default:
                return 0;
        }
    }
}

export enum MouseButton {
    LEFT = 1,
    RIGHT = 2,
    MIDDLE = 4,
}

export function isMouseButtonDown(e: MouseEvent, button: MouseButton): boolean {
    return (getMouseButtons(e) & button) !== 0;
}

// Returns MouseButton for the single button that caused an event
export function getEventOriginButton(e: MouseEvent): MouseButton {
    switch (e.button) {
        case 0:
            return MouseButton.LEFT;
        case 1:
            return MouseButton.MIDDLE;
        case 2:
            return MouseButton.RIGHT;
        default:
            throw new Error(`Unrecognized button: ${e.button}`);
    }
}

// TODO: class for rectangle
export function getRelativeMousePosition(
    event: MouseEvent,
    relativeTo: Element | Lib.Rectangle,
    clampToElement = false,
): Lib.Vector2 {
    const rect: Lib.Rectangle = (() => {
        if (relativeTo instanceof Element) {
            return new Lib.Rectangle(relativeTo.getBoundingClientRect());
        } else {
            return relativeTo;
        }
    })();
    let x = event.clientX - rect.x;
    let y = event.clientY - rect.y;
    // assert(x >= 0 && x <= rect.width, "X Mouse out of bounds: x = " + x + ", width = " + rect.width);
    // assert(y >= 0 && y <= rect.height, "Y Mouse out of bounds: y = " + y + ", height = " + rect.height);
    if (clampToElement) {
        x = Math.min(Math.max(x, 0), rect.sizeX - 1);
        y = Math.min(Math.max(y, 0), rect.sizeY - 1);
    }
    return new Lib.Vector2(x, y);
}
