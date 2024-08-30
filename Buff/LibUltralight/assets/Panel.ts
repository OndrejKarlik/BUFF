import * as Lib from "./Lib.js";
import { DockedPanel } from "./Panel.DockedPanel.js";
import { FloatingPanel } from "./Panel.FloatingPanel.js";

export * from "./Panel.DockedPanel.js";
export * from "./Panel.FloatingPanel.js";

// TODO - later:
// Listen to tabGroupClose - do we need it? It seems it is impossible to close TabElement - we close holding
// DockedPanel instead Do we need tabGroupActiveChildrenChange? AFAIK It was used to set names of DockedPanel,
// maybe we need it to set its DOM name Listen to dockedPanelChildrenChange Simplify hierarchy of the whole
// thing when DockedPanel holds only 1 DockedPanel (in DockedPanel.removeItem)

/*

Hierarchy x holds --> y:
Tabgroup      --> 1-n Tabs
DockedPanel   --> (1 TabGroup | 2 DockedPanels)
FloatingPanel --> DockedPanel
HTML          --> (FloatingPanel | DockedPanel)
*/

export interface DockTabEventDetail {
    tab: Tab;
    mousePos: Lib.Vector2;
}

declare global {
    interface GlobalEventHandlersEventMap {
        tabClose: CustomEvent<Tab>;
        tabActivate: CustomEvent<Tab>;

        tabGroupClose: CustomEvent<TabGroup>;
        tabGroupActiveChildrenChange: CustomEvent<TabGroup>;

        dockedPanelDetach: CustomEvent<DockedPanel>;
        dockedPanelChildrenChange: CustomEvent<DockedPanel>;

        floatingPanelTabDock: CustomEvent<DockTabEventDetail>;

        floatingPanelBodyDockMouse: CustomEvent<Lib.Vector2 | null>;
        floatingPanelBodyDockDrop: CustomEvent<DockedPanel>;
    }
}

// ===========================================================================================================
// Tab
// ===========================================================================================================

export class Tab {
    readonly #name: string;

    readonly #headerElement: HTMLDivElement;
    readonly #contentElement: HTMLDivElement;

    constructor(name: string, content: string) {
        this.#name = name;
        this.#headerElement = Lib.createDomElement(`
            <div class="headerItem">
                ${name}
                <button class="close">╳</button>
            </div>`);
        this.#contentElement = document.createElement("div");
        this.#contentElement.style.height = "100%";
        this.#contentElement.style.width = "100%";
        this.#contentElement.innerHTML = content;

        Lib.assertNonNull(this.#headerElement.querySelector("button.close")).addEventListener(
            "click",
            (e: Event) => {
                e.stopPropagation();
                this.#fireCloseEvent();
            },
        );
        this.#headerElement.addEventListener("mousedown", (e: MouseEvent) => {
            if (Lib.getEventOriginButton(e) === Lib.MouseButton.MIDDLE) {
                this.#fireCloseEvent();
            }
        });

        this.#headerElement.addEventListener("click", () => {
            this.#fireActivateEvent();
        });
    }

    getName(): string {
        return this.#name;
    }
    getContentElement(): HTMLDivElement {
        return this.#contentElement;
    }
    getHeaderElement(): HTMLDivElement {
        return this.#headerElement;
    }

    markAsActive(active: boolean): void {
        if (active) {
            this.#headerElement.classList.add("active");
        } else {
            this.#headerElement.classList.remove("active");
        }
    }

    serializeState(): string {
        return `{ "type": "tab", "name": "${this.#name}" }`;
    }

    // =======================================================================================================
    // Tab: Private methods
    // =======================================================================================================

    #fireCloseEvent(): void {
        const event = new CustomEvent<Tab>("tabClose", { bubbles: true, detail: this });
        this.#headerElement.dispatchEvent(event);
    }
    #fireActivateEvent(): void {
        console.log("Tab.#fireActivateEvent");
        const event = new CustomEvent<Tab>("tabActivate", { bubbles: true, detail: this });
        this.#headerElement.dispatchEvent(event);
    }
}

// ===========================================================================================================
// Tab group
// ===========================================================================================================

interface TabEntry {
    tab: Tab;
    stopTracking: () => void;
}

/// Holds 1-n Tabs
export class TabGroup {
    readonly #tabs: Lib.DomSynchronizedArray<TabEntry>;
    #active: Tab | null = null;

    readonly #element: HTMLDivElement;
    readonly #contentElement: HTMLDivElement;
    readonly #headerElement: HTMLDivElement;

    constructor() {
        this.#element = Lib.createDomElement(`
            <div class="tabGroup">
                <div class="header attachHeaderTarget" style="display: none">
                </div>
                <div class="content"></div>
            </div>`);
        this.#contentElement = Lib.safeCast<HTMLDivElement>(this.#element.querySelector("div.content"));
        this.#headerElement = Lib.safeCast<HTMLDivElement>(this.#element.querySelector("div.header"));

        this.#headerElement.addEventListener("tabClose", (e: CustomEvent<Tab>) => {
            this.detachTab(e.detail);
        });
        this.#headerElement.addEventListener("tabActivate", (e: CustomEvent<Tab>) => {
            console.log("Caught tabActivate");
            this.#activate(e.detail);
        });
        this.#element.addEventListener("floatingPanelTabDock", (e: CustomEvent<DockTabEventDetail>) => {
            const tab: Tab = e.detail.tab;
            console.log(`Attaching to '${this.getName()}'...`);
            this.addTab(tab);

            // First we position the tab onto the mouse so we can call reorderTab, after that we
            // need to position it again.
            tab.getHeaderElement().style.position = "relative";
            tab.getHeaderElement().style.left = "0";
            tab.getHeaderElement().style.top = "0";
            Lib.centerElementAt(tab.getHeaderElement(), e.detail.mousePos);
            tab.getHeaderElement().style.top = "0";

            this.#reorderTab(tab, e.detail.mousePos.x);
            Lib.centerElementAt(tab.getHeaderElement(), e.detail.mousePos);
            tab.getHeaderElement().style.top = "0";

            let myEvent = new MouseEvent("mousedown", {
                clientX: e.detail.mousePos.x,
                clientY: e.detail.mousePos.y,
            });
            tab.getHeaderElement().dispatchEvent(myEvent);
            // Mouse move is necessary here to actually trigger the start callback of the drag tracking
            myEvent = new MouseEvent("mousemove", myEvent);
            tab.getHeaderElement().dispatchEvent(myEvent);
        });

        this.#tabs = new Lib.DomSynchronizedArray<TabEntry>(
            this.#headerElement,
            (item: TabEntry): HTMLElement => item.tab.getHeaderElement(),
        );
    }

    serializeState(): string {
        let children = "";
        this.#tabs.iterate((i) => {
            children += `${i.tab.serializeState()},\n`;
        });
        if (children.endsWith(",\n")) {
            children = children.slice(0, -2);
        }
        return `{ "type": "tabGroup", "tabs": [ ${children} ] }`;
    }

    detachTab(what: Tab): void {
        Lib.assert(this.#containsTab(what), "Trying to detach tab not present in tab group!");
        console.log("TabGroup::detachTab:", what);

        const indexOf = this.#findTab(what);
        if (what === this.#active) {
            if (indexOf < this.#tabs.size() - 1) {
                this.#activate(this.#tabs.get(indexOf + 1).tab);
            } else if (indexOf > 0) {
                this.#activate(this.#tabs.get(indexOf - 1).tab);
            }
        }
        this.#tabs.get(indexOf).stopTracking();
        this.#tabs.splice(indexOf, 1); // Remove
        this.#afterTabCountUpdate();
    }

    addTab(tab: Tab): void {
        Lib.assert(!this.#containsTab(tab), "Trying to attach already present tab");
        console.log(`TabGroup::addTab: ${tab.getName()}`);

        const stopTracking = Lib.trackDragging(
            tab.getHeaderElement(),
            // On start
            (e: MouseEvent) => {
                if (this.#tabs.size() > 1) {
                    if (tab.getHeaderElement().style.position === "relative") {
                        tab.getHeaderElement().style.opacity = "0.5";
                        console.log("TAB_GROUP: Starting attaching header item drag");
                    } else {
                        console.log("TAB_GROUP: Starting header item drag");
                        // It is possible this was alraedy set, together with offsets, if the drag was
                        // initiated by attach operation
                        tab.getHeaderElement().style.position = "relative";
                        tab.getHeaderElement().style.left = "0px";
                        tab.getHeaderElement().style.top = "0px";
                        const capture = tab.getHeaderElement();
                        Lib.animateInterpolate01(200, (t) => {
                            capture.style.opacity = Lib.toStr(1 - t / 2);
                        });
                        console.log(e);
                    }
                    return true;
                } else {
                    console.log(e);
                    return false;
                }
            },
            // On move
            (e: MouseEvent, dX: number /* , dY: number */) => {
                e.stopPropagation();
                tab.getHeaderElement().style.left = `${Lib.convertToPx(tab.getHeaderElement().style.left) + dX}px`;
                this.#reorderTab(tab, e.clientX);
                if (
                    Lib.intersectsRect(
                        Lib.enlargeRect(new Lib.Rectangle(this.#headerElement.getBoundingClientRect()), 10),
                        new Lib.Vector2(e.clientX, e.clientY),
                    )
                ) {
                    return true;
                } else {
                    // Detach. We will do everything right after this callback ends so we do not have 2 drags
                    // happening at the same time, which causes confusing logging and possible interference.
                    setTimeout(() => {
                        console.log("Detaching Floating panel from TabGroup...");
                        this.detachTab(tab);

                        const newTabGroup = new TabGroup();
                        newTabGroup.addTab(tab);

                        const panel = new FloatingPanel(new DockedPanel(newTabGroup));
                        // TODO: Do not access the panel DOM directly here
                        const pivotPoint = new Lib.Vector2(
                            panel.getDomElement().clientWidth / 2,
                            panel.getDomElement().children[0].clientHeight / 2,
                        );
                        Lib.centerElementAt(
                            panel.getDomElement(),
                            new Lib.Vector2(e.clientX, e.clientY),
                            pivotPoint,
                        );
                        const myEvent = new MouseEvent("mousedown", e);
                        Lib.assertNonNull(document.elementFromPoint(e.clientX, e.clientY)).dispatchEvent(
                            myEvent,
                        );
                    });
                    return false;
                }
            },
            // On end
            (/* e: MouseEvent */) => {
                tab.getHeaderElement().style.position = "";
                const capture = tab.getHeaderElement();
                Lib.animateInterpolate01(200, (t) => {
                    capture.style.opacity = Lib.toStr(0.5 + t / 2);
                });
            },
            "TAB_GROUP -> reordering, detaching tabs",
        );

        this.#tabs.push({ tab, stopTracking });
        tab.markAsActive(false);
        this.#afterTabCountUpdate();
    }

    getTabCount(): number {
        return this.#tabs.size();
    }

    getDomElement(): HTMLDivElement {
        return this.#element;
    }

    getName(): string {
        return Lib.assertNonNull(this.#active).getName();
    }

    getTab(index: number): Tab {
        return this.#tabs.get(index).tab;
    }

    // =======================================================================================================
    // TabGroup: Private methods
    // =======================================================================================================

    #findTab(what: Tab): number {
        return this.#tabs.findIndex((i) => i.tab === what);
    }
    #containsTab(what: Tab): boolean {
        return this.#tabs.some((i) => i.tab === what);
    }

    #afterTabCountUpdate(): void {
        if (this.getTabCount() === 0) {
            this.#fireCloseEvent();
        } else if (this.getTabCount() === 1) {
            this.#headerElement.style.display = "none";
            this.#activate(this.#tabs.get(0).tab);
        } else {
            this.#headerElement.style.display = "";
        }
    }

    #fireCloseEvent(): void {
        console.log("TabGroup::fireCloseEvent");
        const event = new CustomEvent<TabGroup>("tabGroupClose", { bubbles: true, detail: this });
        this.#element.dispatchEvent(event);
    }

    #activate(what: Tab): void {
        Lib.assert(this.#containsTab(what), "Trying to activate tab not present in this tab group!");
        this.#tabs.iterate((i) => {
            i.tab.markAsActive(false);
        });
        what.markAsActive(true);
        this.#contentElement.innerHTML = "";
        this.#contentElement.appendChild(what.getContentElement());
        this.#active = what;
        const event = new CustomEvent<TabGroup>("tabGroupActiveChildrenChange", {
            bubbles: true,
            detail: this,
        });
        this.#element.dispatchEvent(event);
    }

    // eslint-disable-next-line no-unused-private-class-members
    #getMinimumWidth(): number {
        let result = 0;
        this.#tabs.iterate((i) => {
            result += Lib.getElementSize(i.tab.getContentElement(), Lib.SizeType.WITH_MARGIN).x;
        });
        return result;
    }

    #reorderTab(tab: Tab, tabX: number): void {
        Lib.assert(this.#containsTab(tab), "Trying to reorder tab not present in this tab group!");
        const tabHeader = tab.getHeaderElement();
        const inputTabIndex = this.#findTab(tab);
        let insertBefore: number | undefined = undefined;
        for (let i = 0; i < inputTabIndex; ++i) {
            // Try moving to the left
            const rect = new Lib.Rectangle(this.#tabs.get(i).tab.getHeaderElement().getBoundingClientRect());
            if (tabX < rect.center().x) {
                insertBefore = i;
                break;
            }
        }
        for (let i = this.#tabs.size() - 1; i > inputTabIndex; i--) {
            // Try moving to the right
            const rect = new Lib.Rectangle(this.#tabs.get(i).tab.getHeaderElement().getBoundingClientRect());
            if (tabX > rect.center().x) {
                Lib.assert(insertBefore === undefined, "Trying to insert before and after at the same time");
                insertBefore = i + 1;
                break;
            }
        }
        if (insertBefore !== undefined) {
            console.log(`Inserting ${inputTabIndex} before ${insertBefore}`);
            const originalOffset = tabHeader.offsetLeft;
            this.#tabs.move(inputTabIndex, insertBefore);
            tabHeader.style.left = `${Lib.convertToPx(tabHeader.style.left) + originalOffset - tabHeader.offsetLeft}px`;
        }
    }
}

// ===========================================================================================================
// Helpers to call from C++
// ===========================================================================================================

// CALLED FROM C++
export function addFloatingPanelBase64(id: string, titleBase64: string, contentBase64: string): void {
    const title = atob(titleBase64);
    const content = atob(contentBase64);

    const tabGroup = new TabGroup();
    const tab = new Tab(title, content);
    // const scriptTag = tab.getContent().querySelector("script");
    // Lib.assert(scriptTag);
    tabGroup.addTab(tab);
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    const floating = new FloatingPanel(new DockedPanel(tabGroup));
    // const script   = document.createElement("script");
    // script.text    = scriptTag.text;
    // document.body.appendChild(script);
    // floating.id = id;
}
