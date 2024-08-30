import * as Lib from "./Lib.js";
import { FloatingPanel, TabGroup } from "./Panel.js";

enum DockPosition {
    LEFT,
    RIGHT,
    UP,
    DOWN,
}

enum DockedPanelOrientation {
    HORIZONTAL = "horizontal",
    VERTICAL = "vertical",
}

type DockedPanelChild = DockedPanel | TabGroup;

// Has either 1 child (a tab group), or 2+ children (docked panels)
export class DockedPanel {
    readonly #children: Lib.DomSynchronizedArray<DockedPanelChild>;
    #orientation: DockedPanelOrientation = DockedPanelOrientation.HORIZONTAL;

    readonly #element: HTMLDivElement;
    readonly #contentElement: HTMLDivElement;
    readonly #headerElement: HTMLDivElement;
    readonly #dockElement: HTMLDivElement;

    constructor(initial: DockedPanelChild | null) {
        this.#element = Lib.createDomElement(`
            <div class="dockedPanel">
                <div class="header">
                    <span></span>
                    <div class="buttons"><button class="close">╳</button></div>
                </div>
                <div class="dockTarget">
                    <div class="dockArrow up"  style="bottom: 30px;">▲</div>
                    <div class="dockArrow left" style="right: 30px;">◀</div>
                    <div class="dockArrow right"  style="left: 30px;">▶</div>
                    <div class="dockArrow down"    style="top: 30px;">▼</div>
                </div>
                <div class="content"></div>
            </div>`);
        this.#contentElement = Lib.safeCast<HTMLDivElement>(this.#element.querySelector("div.content"));
        this.#headerElement = Lib.safeCast<HTMLDivElement>(this.#element.querySelector("div.header"));
        this.#dockElement = Lib.safeCast<HTMLDivElement>(this.#element.querySelector("div.dockTarget"));
        Lib.assertNonNull(this.#headerElement.querySelector("button.close")).addEventListener("click", () => {
            this.#deleteThis();
        });

        this.#children = new Lib.DomSynchronizedArray<DockedPanelChild>(
            this.#contentElement,
            (x: DockedPanelChild): HTMLElement => x.getDomElement(),
        );

        let dragOrigin: Lib.Vector2 | null = null;
        Lib.trackDragging(
            this.#headerElement,
            (e: MouseEvent) => {
                dragOrigin = new Lib.Vector2(e.clientX, e.clientY);
                return true;
            },
            // On move
            (e: MouseEvent) => {
                Lib.assert(dragOrigin !== null);
                if (Math.max(Math.abs(dragOrigin.x - e.clientX), Math.abs(dragOrigin.y - e.clientY)) > 20) {
                    console.log("Detaching...");
                    const rect = this.#element.getBoundingClientRect();
                    const pivot = new Lib.Vector2(dragOrigin.x - rect.left, dragOrigin.y - rect.top);

                    setTimeout(() => {
                        Lib.assertNonNull(this.#element.parentElement).dispatchEvent(
                            new CustomEvent<DockedPanel>("dockedPanelDetach", {
                                bubbles: true,
                                detail: this,
                            }),
                        );

                        const panel = new FloatingPanel(this);
                        pivot.x = Math.min(pivot.x, panel.getDomElement().getBoundingClientRect().width - 10);
                        Lib.centerElementAt(
                            panel.getDomElement(),
                            new Lib.Vector2(e.clientX, e.clientY),
                            pivot,
                        );

                        const myEvent = new MouseEvent("mousedown", e);
                        // TODO: Do not access the panel DOM directly here
                        panel.getDomElement().querySelector("div.header")?.dispatchEvent(myEvent);
                    });
                    return false;
                } else {
                    return true;
                }
            },
            undefined,
            "DOCKED_PANEL -> detaching tabs",
        );

        this.#element.addEventListener("dockedPanelDetach", (e: CustomEvent<DockedPanel>) => {
            this.#removeItem(e.detail);
        });
        this.#element.addEventListener("floatingPanelBodyDockMouse", (e: CustomEvent<Lib.Vector2 | null>) => {
            const shouldBeVisible: boolean =
                e.detail !== null &&
                this.#getItemCount() === 1 &&
                Lib.intersectsRect(this.#contentElement.getBoundingClientRect(), e.detail);

            // console.log(`${this.getName()} shouldBeVisible = ${shouldBeVisible} (${
            //    this.#getItemCount()} items, ${e.detail.mousePos} vs ${
            //    new Lib.Rectangle(this.#contentElement.getBoundingClientRect())})`);
            this.#dockElement.style.display = shouldBeVisible ? "block" : "none";
            if (shouldBeVisible) {
                for (const i of this.#dockElement.children) {
                    if (Lib.intersectsRect(i.getBoundingClientRect(), e.detail!)) {
                        i.classList.add("panelHover");
                    } else {
                        i.classList.remove("panelHover");
                    }
                }
            }
        });

        const dropHandler = (e: CustomEvent<DockedPanel>, value: Element): void => {
            const dockPosition: DockPosition = (() => {
                if (value.classList.contains("left")) {
                    return DockPosition.LEFT;
                } else if (value.classList.contains("right")) {
                    return DockPosition.RIGHT;
                } else if (value.classList.contains("up")) {
                    return DockPosition.UP;
                } else {
                    Lib.assert(value.classList.contains("down"));
                    return DockPosition.DOWN;
                }
            })();
            console.log(`DROPPING - attach body ${dockPosition}`);
            this.#addItem(e.detail, dockPosition === DockPosition.LEFT || dockPosition === DockPosition.UP);
            this.#setOrientation(
                dockPosition === DockPosition.LEFT || dockPosition === DockPosition.RIGHT
                    ? DockedPanelOrientation.HORIZONTAL
                    : DockedPanelOrientation.VERTICAL,
            );
        };
        this.#element.querySelectorAll("div.dockArrow").forEach((value: Element) => {
            Lib.safeCast<HTMLElement>(value).addEventListener("floatingPanelBodyDockDrop", (e) => {
                dropHandler(e, value);
            });
        });

        if (initial !== null) {
            this.#addItem(initial, true);
        }
    }

    serializeState(): string {
        let children = "";
        this.#children.iterate((i: DockedPanelChild) => {
            children += `${i.serializeState()},\n`;
        });
        children = children.slice(0, -2);
        return `{ "type": "dockedPanel", "orientation" : "${this.#orientation}",  "children": [ ${children} ] }`;
    }

    getDomElement(): HTMLDivElement {
        return this.#element;
    }

    getName(): string {
        return this.#children.get(0).getName();
    }

    getItem(index: number): DockedPanelChild {
        return this.#children.get(index);
    }

    // =======================================================================================================
    // DockedPanel: Private methods
    // =======================================================================================================

    #addItem(item: DockedPanelChild, isFirst: boolean): void {
        Lib.assert(!this.#children.contains(item), "Trying to add item already present");

        if (this.#getItemCount() === 0) {
            //Lib.assert(dockedPosition === undefined, "Position must not be specified for the first item.");
            //Lib.assert(size === undefined, "Size can be only specified for the second item");
            this.#children.push(item);
        } else {
            Lib.assert(this.#getItemCount() === 1);
            //Lib.assert(
            //    dockedPosition !== undefined,
            //    "Position needs to be specified for adding more than 1 item.",
            //);

            // We need to wrap everything into docked panels
            const firstChild = this.#children.get(0);
            if (firstChild instanceof TabGroup) {
                this.#children.remove(0);
                this.#children.push(new DockedPanel(firstChild));
            }
            if (item instanceof TabGroup) {
                item = new DockedPanel(item);
            }
            this.#children.splice(isFirst ? 0 : 1, 0, item);

            this.#element.classList.add("multi");
            //const horizontal = dockedPosition === DockPosition.LEFT || dockedPosition === DockPosition.RIGHT;
            //this.#element.classList.add(horizontal ? "horizontal" : "vertical");

            //if (size !== undefined) {
            //    this.#children.get(0).getDomElement().style.flex = "1 1 auto";
            //    this.#children.get(1).getDomElement().style.flex = `0 0 ${size * 100}%`;
            //}
        }
        this.#onChildrenChange();
    }

    #setOrientation(orientation: DockedPanelOrientation): void {
        this.#orientation = orientation;
        if (orientation === DockedPanelOrientation.HORIZONTAL) {
            this.#element.classList.remove("vertical");
            this.#element.classList.add("horizontal");
        } else {
            this.#element.classList.add("vertical");
            this.#element.classList.remove("horizontal");
        }
    }

    #onChildrenChange(): void {
        if (this.#getItemCount() === 1 && this.#children.get(0) instanceof TabGroup) {
            this.#headerElement.style.display = "none";
        } else {
            this.#headerElement.style.display = "block";
        }

        const event = new CustomEvent<DockedPanel>("dockedPanelChildrenChange", {
            bubbles: true,
            detail: this,
        });
        this.#element.dispatchEvent(event);
    }

    #deleteThis(): void {
        console.log(`DockedPanel::delete: ${this.getName()}`);
        Lib.assertNonNull(this.#element.parentElement).dispatchEvent(
            new CustomEvent<DockedPanel>("dockedPanelDetach", { bubbles: true, detail: this }),
        );
    }

    #removeItem(what: DockedPanelChild): void {
        console.log(this.#children);
        console.log(what);
        // Lib.assert(this.#getItemCount() === 2, "Detach only supported when there are 2 children");
        Lib.assert(this.#children.contains(what), "Trying to remove child not present in docked panel");
        console.log("DockedPanel::removeItem:", what);
        // const index = this.#children.indexOf(what);
        this.#children.splice(this.#children.indexOf(what), 1);
        this.#element.classList.remove("multi");
        this.#element.classList.remove("horizontal");
        this.#element.classList.remove("vertical");
        // If the parent is also DockedPanel (not FloatingPanel), we will simplify the hierarchy by
        // removing this unnecessary step
        const onlyChild = this.#children.get(0);
        this.#children.remove(0);
        if (onlyChild instanceof DockedPanel) {
            console.log("Simplifying hierarchy");
            this.#setOrientation(onlyChild.#orientation);
            while (onlyChild.#getItemCount() > 0) {
                const child = onlyChild.getItem(0);
                onlyChild.#children.remove(0);
                this.#addItem(child, true);
            }
        }
        // if (element.getItem(0).IS_DOCKED_PANEL) {
        //
        //    element.replaceItem(element.getItem(0).getItem(0), element.getItem(0));
        // }
        this.#onChildrenChange();
    }

    // #replaceItem(item: DockedPanelChild, oldItem: DockedPanelChild): void {
    //     Lib.assert(Lib.contains(this.#getAllItems(), oldItem));
    //     this.#contentElement.replaceChild(item.getDomElement(), oldItem.getDomElement());
    //     this.#onChildrenChange();
    // }
    #getItemCount(): number {
        return this.#children.size();
    }
}
