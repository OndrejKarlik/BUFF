import * as Lib from "./Lib.js";
import type { DockedPanel, DockTabEventDetail } from "./Panel.js";
// eslint-disable-next-line no-duplicate-imports
import { TabGroup } from "./Panel.js";

// ===========================================================================================================
// Floating panel
// ===========================================================================================================

let gLastPanelZIndex = 100;

enum Resize {
    NONE,
    RIGHT,
    BOTTOM,
    BOTH,
}

export class FloatingPanel {
    readonly #element: HTMLDivElement;
    readonly #contentElement: HTMLDivElement;
    readonly #headerElement: HTMLDivElement;

    #content: DockedPanel | null = null;
    #name: string | null = null;

    constructor(childPanel: DockedPanel) {
        this.#element = Lib.createDomElement(`
            <div class="floatingPanel">
                <div class="header">
                    <span></span>
                    <div class="buttons"><button class="close">╳</button></div>
                </div>
                <div class="content">
                </div>
            </div>`);

        this.#contentElement = Lib.safeCast<HTMLDivElement>(this.#element.querySelector("div.content"));
        this.#headerElement = Lib.safeCast<HTMLDivElement>(this.#element.querySelector("div.header"));

        this.#element.addEventListener("mousedown", () => {
            this.#bringToTop();
        });
        Lib.assertNonNull(this.#element.querySelector("button.close")).addEventListener("click", () => {
            this.#deleteThis();
        });

        // ===================================================================================================
        // Mouse resizing
        // ===================================================================================================

        let resizeHover = Resize.NONE;

        // Tracking move moving into resizing areas
        this.#element.addEventListener("mousemove", (e: MouseEvent) => {
            const TOLERANCE = 10;
            const isRight = e.clientX > this.#element.getBoundingClientRect().right - TOLERANCE;
            const isBottom = e.clientY > this.#element.getBoundingClientRect().bottom - TOLERANCE;
            if (isRight && isBottom) {
                this.#element.style.cursor = "nwse-resize";
                resizeHover = Resize.BOTH;
            } else if (isRight) {
                this.#element.style.cursor = "ew-resize";
                resizeHover = Resize.RIGHT;
            } else if (isBottom) {
                this.#element.style.cursor = "ns-resize";
                resizeHover = Resize.BOTTOM;
            } else {
                this.#element.style.cursor = "";
                resizeHover = Resize.NONE;
            }
        });

        let resizeActual = Resize.NONE;
        let draggingPosition: Lib.Vector2 | null = null;

        // Handle resizing
        Lib.trackDragging(
            this.#element,
            () => {
                console.log("Trying resize...");
                if (resizeHover === Resize.NONE) {
                    console.log("Abort resize...");
                    return false;
                }
                resizeActual = resizeHover;
                draggingPosition = Lib.getElementSize(this.#element, Lib.SizeType.CONTENT);
                Lib.overrideCss(`* { cursor: ${this.#element.style.cursor} } `);
                return true;
            },
            (e: MouseEvent, dX: number, dY: number) => {
                Lib.assert(draggingPosition !== null);
                e.stopPropagation();
                draggingPosition.x += dX;
                draggingPosition.y += dY;
                if (resizeActual === Resize.RIGHT || resizeActual === Resize.BOTH) {
                    this.#element.style.width = `${draggingPosition.x}px`;
                }
                if (resizeActual === Resize.BOTTOM || resizeActual === Resize.BOTH) {
                    this.#element.style.height = `${draggingPosition.y}px`;
                }
                return true;
            },
            () => {
                Lib.endOverrideCss();
            },
            "FLOATING_PANEL -> resizing",
        );

        // ===================================================================================================
        // Dragging - move panel, possibly attach.
        // ===================================================================================================

        const notifyBodyDropTargets = (mousePos: Lib.Vector2 | null): void => {
            const dockEvent = new CustomEvent<Lib.Vector2 | null>("floatingPanelBodyDockMouse", {
                detail: mousePos,
            });
            document.querySelectorAll("div.dockedPanel").forEach((i) => i.dispatchEvent(dockEvent));
        };

        Lib.trackDragging(
            this.#headerElement,
            () => {
                // On start
                Lib.animateInterpolate01(200, (t) => {
                    this.#element.style.opacity = Lib.toStr(1 - t / 2);
                });
                if (this.#isDockableToHeader()) {
                    Lib.overrideCss(`div.tabGroup > div.header {
                                 background-color: var(--dockTarget) !important;
                             }`);
                }
                console.log(`FLOATING_PANEL ${this.#getName()}: started dragging`);
                return true;
            },
            (e: MouseEvent, dX: number, dY: number) => {
                // On move
                this.#element.style.left = `${this.#element.offsetLeft + dX}px`;
                this.#element.style.top = `${this.#element.offsetTop + dY}px`;
                if (this.#isDockableToHeader()) {
                    const dockableAsTab = document
                        .elementsFromPoint(e.clientX, e.clientY)
                        .find(
                            (i) =>
                                i.classList.contains("attachHeaderTarget") &&
                                !Lib.isTransitiveChild(i, this.#element),
                        );
                    if (dockableAsTab) {
                        const tabGroup: TabGroup = Lib.safeCast<TabGroup>(this.#getItem().getItem(0));
                        const event = new CustomEvent<DockTabEventDetail>("floatingPanelTabDock", {
                            bubbles: true,
                            detail: {
                                tab: tabGroup.getTab(0),
                                mousePos: new Lib.Vector2(e.clientX, e.clientY),
                            },
                        });
                        tabGroup.detachTab(tabGroup.getTab(0)); // Necessary to prevent asserts
                        this.#deleteThis();
                        setTimeout(() => dockableAsTab.dispatchEvent(event));
                        return false;
                    }
                }
                notifyBodyDropTargets(new Lib.Vector2(e.clientX, e.clientY));
                return true;
            },
            (e: MouseEvent) => {
                // On end
                Lib.endOverrideCss();
                const target = document
                    .elementsFromPoint(e.clientX, e.clientY)
                    .find((i) => i.classList.contains("dockArrow"));
                // This hides the dock targets, so we need to query the elements under first!
                notifyBodyDropTargets(null);
                if (target) {
                    const dockEvent = new CustomEvent<DockedPanel>("floatingPanelBodyDockDrop", {
                        detail: this.#getItem(),
                    });
                    target.dispatchEvent(dockEvent);
                    this.#deleteThis();
                } else {
                    Lib.animateInterpolate01(200, (t) => {
                        this.#element.style.opacity = Lib.toStr(0.5 + t / 2);
                    });
                }
            },
            "FLOATING_PANEL -> headerElement: moving, attaching",
        );

        this.#replaceItem(childPanel);
        this.#bringToTop();
        this.#element.style.left = "0";
        this.#element.style.top = "0";
        document.body.appendChild(this.#element);
    }

    serializeState(): string {
        return `{ "type": "floatingPanel", "children": [ ${this.#content!.serializeState()} ] }`;
    }

    getDomElement(): HTMLDivElement {
        return this.#element;
    }

    // =======================================================================================================
    // FloatingPanel: Private methods
    // =======================================================================================================

    #getItem(): DockedPanel {
        return Lib.assertNonNull(this.#content);
    }

    #onChildrenChange(): void {
        console.log("FloatingPanel::onChildrenChange");
        this.#setName(this.#getItem().getName());
    }

    #deleteThis(): void {
        console.log("FloatingPanel::delete");
        this.#element.remove();
    }

    #bringToTop(): void {
        this.#element.style.zIndex = Lib.toStr(gLastPanelZIndex++);
    }

    #getName(): string {
        return Lib.assertNonNull(this.#name);
    }
    #setName(name: string): void {
        this.#name = name;
        Lib.safeCast<HTMLElement>(this.#headerElement.children[0]).innerText = name;
    }
    #replaceItem(item: DockedPanel): void {
        this.#content = item;
        this.#contentElement.innerHTML = "";
        this.#contentElement.appendChild(item.getDomElement());
        this.#onChildrenChange();
    }

    #isDockableToHeader(): boolean {
        const childChild = this.#getItem().getItem(0);
        if (childChild instanceof TabGroup) {
            return childChild.getTabCount() === 1;
        } else {
            return false;
        }
    }
}
