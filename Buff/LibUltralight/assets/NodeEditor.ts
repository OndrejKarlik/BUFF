import * as Lib from "./Lib.js";

interface ConnectionRequest {
    connection: Connection;
    type: SlotType;
}

declare global {
    interface GlobalEventHandlersEventMap {
        connectToSlot: CustomEvent<ConnectionRequest>;
        deleteConnection: CustomEvent<Connection>;
    }
}

const SVG_NS = "http://www.w3.org/2000/svg";

export enum SlotType {
    INPUT = "input",
    OUTPUT = "output",
}
function getOppositeSlotType(type: SlotType): SlotType {
    return type === SlotType.INPUT ? SlotType.OUTPUT : SlotType.INPUT;
}

class Connection {
    #input: Lib.Vector2 = new Lib.Vector2(0, 0);
    #output: Lib.Vector2 = new Lib.Vector2(0, 0);
    #inputSlot: Slot | null = null;
    #outputSlot: Slot | null = null;
    readonly #element: SVGPathElement;

    constructor() {
        this.#element = Lib.safeCast<SVGPathElement>(document.createElementNS(SVG_NS, "path"));
        this.#element.setAttribute("stroke", "black");
        this.#element.setAttribute("stroke-width", "2");
        this.#element.setAttribute("fill", "none");
        this.#element.style.pointerEvents = "all";

        this.#element.addEventListener("mouseenter", () => {
            this.#element.setAttribute("stroke", "red");
        });
        this.#element.addEventListener("mouseleave", () => {
            this.#element.setAttribute("stroke", "black");
        });
        this.#element.addEventListener("contextmenu", (e) => {
            Lib.showContextMenu(e, [
                {
                    name: "Delete connection",
                    action: (): void => {
                        if (this.#inputSlot) {
                            this.#inputSlot.removeConnection(this);
                        }
                        if (this.#outputSlot) {
                            this.#outputSlot.removeConnection(this);
                        }
                    },
                },
            ]);
        });
    }

    getDomElement(): SVGPathElement {
        return this.#element;
    }

    /// \param coords
    /// Provided in parent SVG coordinates
    setCoords(coords: Lib.Vector2, type: SlotType): void {
        // console.log(`setCoords: ${end} -> ${coords}`);
        if (type === SlotType.OUTPUT) {
            this.#output = coords;
        } else {
            this.#input = coords;
        }
        this.#update();
        // console.log("setcoords " + coords + " | " + isOutput);
    }

    // Used only to observe, do NOT use this to mutate the state
    updateSlot(slot: Slot | null, type: SlotType): void {
        if (type === SlotType.OUTPUT) {
            this.#outputSlot = slot;
        } else {
            this.#inputSlot = slot;
        }
        if (!this.#inputSlot && !this.#outputSlot) {
            this.#element.remove();
        }
    }
    getSlot(type: SlotType): Slot | null {
        return type === SlotType.OUTPUT ? this.#outputSlot : this.#inputSlot;
    }

    #update(): void {
        // const svgCorner = [svg.getBoundingClientRect().left, svg.getBoundingClientRect().top];
        // const a = [connection.mInput.x - svgCorner.x, connection.mInput.y - svgCorner.y];
        // const b = [connection.mOutput.x - svgCorner.x, connection.mOutput.y - svgCorner.y];
        const a = this.#input; // TODO: handle the coordinate system
        const b = this.#output;
        const handleLength = (b.x - a.x) / 2;
        const pathData = `M ${a.x} ${a.y} C ${a.x + handleLength} ${a.y}, ${b.x - handleLength} ${b.y}, ${b.x} ${b.y}`;
        this.#element.setAttribute("d", pathData);
    }
}

class Slot {
    readonly #type: SlotType;
    // eslint-disable-next-line no-unused-private-class-members
    readonly #name: string;
    readonly #element: HTMLDivElement;
    readonly #handleElement: HTMLDivElement;
    readonly #connections: Connection[] = [];
    readonly #parent: Node;

    constructor(name: string, type: SlotType, parent: Node) {
        this.#name = name;
        this.#type = type;
        this.#parent = parent;
        const html: string = (function () {
            if (type === SlotType.OUTPUT) {
                return `<div class="output item">
                            <div style="flex-grow: 1;">
                            </div>
                            ${name}
                            <div class="handle">
                            </div>
                        </div>`;
            } else {
                return `<div class="input item">
                            <div class="handle">
                            </div>
                            ${name}
                        </div>`;
            }
        })();
        this.#element = Lib.createDomElement(html);

        this.#handleElement = Lib.querySelectorUnique<HTMLDivElement>("div.handle", this.#element);

        this.#handleElement.addEventListener("connectToSlot", (e: CustomEvent<ConnectionRequest>): void => {
            if (
                e.detail.type === this.getType() &&
                e.detail.connection.getSlot(getOppositeSlotType(e.detail.type))?.getParentNode() !==
                    this.getParentNode()
            ) {
                this.addConnection(e.detail.connection);
            }
        });

        this.#element.addEventListener("deleteConnection", (e: CustomEvent<Connection>): void => {
            this.removeConnection(e.detail);
        });

        this.#handleElement.addEventListener("mousedown", (mouseDownEvent: MouseEvent) => {
            // console.log("MOUSEDOWN");

            mouseDownEvent.stopPropagation();
            const offset = this.getRootSvg().getBoundingClientRect();
            const connection: Connection = new Connection();
            this.addConnection(connection);

            const mouseMove = (e: MouseEvent): void => {
                connection.setCoords(
                    new Lib.Vector2(e.clientX - offset.x, e.clientY - offset.y),
                    getOppositeSlotType(this.getType()),
                );
            };
            const mouseUp = (e: MouseEvent): void => {
                console.log("MouseUp!");
                document.removeEventListener("mousemove", mouseMove);
                document.removeEventListener("mouseup", mouseUp);

                const targetHandle = document
                    .elementsFromPoint(e.clientX, e.clientY)
                    .find((i) => i.classList.contains("handle"));
                if (targetHandle) {
                    // console.log("Connecting the other side!");
                    const event = new CustomEvent<ConnectionRequest>("connectToSlot", {
                        detail: { connection, type: getOppositeSlotType(this.getType()) },
                    });
                    targetHandle.dispatchEvent(event);
                }
                if (!connection.getSlot(getOppositeSlotType(this.getType()))) {
                    this.removeConnection(connection);
                }
            };
            document.addEventListener("mousemove", mouseMove);
            document.addEventListener("mouseup", mouseUp);
            mouseMove(mouseDownEvent); // Set initial coords of the other end
            this.getRootSvg().appendChild(connection.getDomElement());
        });
    }

    getRootSvg(): Element {
        return Lib.querySelectorUnique(
            "svg.connections",
            Lib.assertNonNull(this.#element.closest("div.nodeEditor")),
        );
    }

    addConnection(connection: Connection): void {
        Lib.assert(!Lib.contains(this.#connections, connection), "trying to add the same connection twice");
        connection.updateSlot(this, this.getType());
        if (this.getType() === SlotType.INPUT) {
            // Only one connection to inputs
            Lib.assert(this.#connections.length <= 1);
            if (this.#connections.length === 1) {
                const slot = this.#connections[0].getSlot(getOppositeSlotType(this.getType()));
                if (slot) {
                    slot.removeConnection(this.#connections[0]);
                }
                this.removeConnection(this.#connections[0]);
            }
        }
        this.#connections.push(connection);
        this.#handleElement.classList.add("connected");
        this.updateConnectionCoords();
    }

    removeConnection(connection: Connection): void {
        const index = this.#connections.indexOf(connection);
        Lib.assert(index >= 0, "trying to remove non-existent connection");
        this.#connections.splice(index, 1);
        connection.updateSlot(null, this.getType());
        if (this.#connections.length === 0) {
            this.#handleElement.classList.remove("connected");
        }
    }

    getType(): SlotType {
        return this.#type;
    }
    updateConnectionCoords(): void {
        // console.log("updateConnectionCoords");
        for (const connection of this.#connections) {
            const rect = this.getRootSvg().getBoundingClientRect();
            const coords = Lib.getElementCenter(this.#handleElement);
            connection.setCoords(new Lib.Vector2(coords.x - rect.x, coords.y - rect.y), this.getType());
        }
    }
    getDomElement(): HTMLDivElement {
        return this.#element;
    }
    getParentNode(): Node {
        return this.#parent;
    }
}

class Node {
    readonly #element: HTMLDivElement;
    readonly #inputs: Lib.DomSynchronizedArray<Slot>;
    readonly #outputs: Lib.DomSynchronizedArray<Slot>;

    constructor(name: string, position: Lib.Vector2) {
        this.#element = Lib.createDomElement<HTMLDivElement>(`
            <div class="node">
                <h1>${name}</h1>
                <div class="inputs"></div>
                <div class="outputs"></div>
            </div>`);
        this.#inputs = new Lib.DomSynchronizedArray<Slot>(
            Lib.querySelectorUnique("div.inputs", this.#element),
            (x: Slot): HTMLElement => x.getDomElement(),
        );
        this.#outputs = new Lib.DomSynchronizedArray<Slot>(
            Lib.querySelectorUnique("div.outputs", this.#element),
            (x: Slot): HTMLElement => x.getDomElement(),
        );

        this.#element.style.left = `${position.x}px`;
        this.#element.style.top = `${position.y}px`;

        const onMove = (): boolean => {
            for (let i = 0; i < this.getInputsCount(); i++) {
                this.getInput(i).updateConnectionCoords();
            }
            for (let i = 0; i < this.getOutputsCount(); i++) {
                this.getOutput(i).updateConnectionCoords();
            }
            return true;
        };
        Lib.makeDraggable(this.#element, undefined, onMove, undefined);
    }

    addSlot(name: string, type: SlotType): Slot {
        const slot = new Slot(name, type, this);
        if (type === SlotType.OUTPUT) {
            this.#outputs.push(slot);
        } else {
            this.#inputs.push(slot);
        }
        return slot;
    }

    getInputsCount(): number {
        return this.#inputs.size();
    }
    getInput(index: number): Slot {
        Lib.assert(
            index >= 0 && index < this.getInputsCount(),
            `Input index out of bounds: accessing ${index} out of ${this.getInputsCount()}`,
        );
        return this.#inputs.get(index);
    }
    getOutputsCount(): number {
        return this.#outputs.size();
    }
    getOutput(index: number): Slot {
        Lib.assert(
            index >= 0 && index < this.getOutputsCount(),
            `Output index out of bounds: accessing ${index} out of ${this.getOutputsCount()}`,
        );
        return this.#outputs.get(index);
    }

    getDomElement(): HTMLDivElement {
        return this.#element;
    }
}

export class NodeEditor {
    #nextZ = 100;
    readonly #nodes: Lib.DomSynchronizedArray<Node>;
    readonly #element: HTMLDivElement;
    // eslint-disable-next-line no-unused-private-class-members
    readonly #svgElement: SVGSVGElement;
    constructor() {
        this.#element = Lib.createDomElement<HTMLDivElement>(`
            <div class="nodeEditor">
                <svg class="connections"></svg>
                <div class="nodes"></div>
            </div>`);
        this.#svgElement = Lib.querySelectorUnique<SVGSVGElement>("svg.connections", this.#element);
        this.#nodes = new Lib.DomSynchronizedArray<Node>(
            Lib.querySelectorUnique("div.nodes", this.#element),
            (x: Node): HTMLElement => x.getDomElement(),
        );
    }

    addNode(name: string, position: Lib.Vector2): Node {
        const res = new Node(name, position);
        this.#nodes.push(res);

        // Using arrow function because it does not rebind "this"
        res.getDomElement().addEventListener("mousedown", () => {
            // console.log(element.nextZ);
            res.getDomElement().style.zIndex = Lib.toStr(this.#nextZ++);
        });

        return res;
    }

    getDomElement(): HTMLDivElement {
        return this.#element;
    }
}
