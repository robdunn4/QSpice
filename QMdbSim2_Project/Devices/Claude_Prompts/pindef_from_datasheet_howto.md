# How to Produce a `.qpindef` File from a Microchip/Atmel Datasheet

This guide describes how to read an MCU datasheet and produce the `.qpindef`
Pin Definitions file consumed by QSymGen3.

---

## 1. Format Reference

Each line begins with a single-character record type code, followed by one
or more whitespace-separated fields. Blank lines and lines beginning with
`*` are comments and are ignored.

| Code | Name          | Syntax                       | Notes |
|------|---------------|-------------------------------|-------|
| `*`  | Comment       | `* any text`                  | Entire line ignored |
| `D`  | Description   | `D <text>`                    | One line, human-readable device description |
| `P`  | Part list     | `P <part> <part> ...`         | Exact part numbers this file covers |
| `R`  | Output resistance | `R <ohms>`                | Optional; sets `ROUT` DLL attribute. Default is **1 Ω** if omitted |
| `I`  | Input pin     | `I <pinName> [altText]`       | Input-only |
| `O`  | Output pin    | `O <pinName> [altText]`       | Output-only |
| `B`  | Bidirectional | `B <pinName> [altText]`       | Generates the three-argument `addPinPortMap()` call (`_I`/`_O`/`_C` suffixes) |
| `V`  | VDD           | `V <pinName>`                 | Exactly one per file |
| `G`  | GND           | `G <pinName>`                 | Exactly one per file. `GND` is rewritten to `GND_` internally to avoid a QSpice reserved-name collision — you don't need to do anything about this yourself |
| `K`  | SimClock      | `K <pinName>`                 | Exactly one per file. **Synthetic** — not in the datasheet. Always `K SIMCLK` |
| `X`  | Spacer        | `X`                            | Layout only, no physical pin |

`D`, `P`, and `R` are file-level records and should precede the pin records.

`altText` is optional free-form text — typically the alternate-function
list from the pinout table — carried through to the generated symbol.

The input file's default extension is `.qpindef`; you can omit it on the
QSymGen3 command line (`QSymGen3 ATtiny85` works the same as
`QSymGen3 ATtiny85.qpindef`).

---

## 2. Symbol Layout Convention

Pin *record order* in the file controls left/right column placement in
the generated `.qsym` symbol: the first half of records become the left
column (top to bottom), the second half become the right column (top to
bottom). Within that constraint, three pins have fixed positions:

- The **positive supply pin** (VDD/VCC/etc.) is the **first record**
  overall — top-left of the symbol.
- **SIMCLK** is the **last record of the left column** — bottom-left.
- The **ground pin** (VSS/GND/VEE/etc.) is the **last record overall** —
  bottom-right.

Everything else fills the remaining slots — see Section 3 for how those
are ordered and Section 4 for a worked example.

### Odd pin counts

If the total record count (including `V`/`G`/`K`) is odd, the left/right
split becomes ambiguous — it depends on how QSymGen3 rounds `size/2`,
which you shouldn't have to guess at. Insert one `X` spacer record
immediately before `SIMCLK` to bring the total to an even number. This
gives an exact half-and-half split and guarantees `SIMCLK` lands as the
last entry of the left column, regardless of rounding direction.

---

## 3. Step-by-Step Procedure

### Step 1 — Fill in `D` and `P`

Pull the description and exact part numbers from the datasheet's title
page and ordering/part-number information section.

### Step 2 — Find `V` and `G`

Locate these in the pinout diagram. There should be exactly one `V`
record and one `G` record per file.

**If a device has more than one physical VDD or GND pin:** don't invent
a new pin name (`VDD2`, `GND2`, etc.) for the extras. Add a comment next
to the primary `V`/`G` record noting the duplicate pin number, and don't
create a second record for it at all — it's the same net, not a second
signal.

### Step 3 — Add `K SIMCLK`

Every device needs exactly one `K SIMCLK` record. This pin is not in the
datasheet — it's synthetic and added by convention.

### Step 4 — Classify every remaining (GPIO) pin

Read the port-description section(s) of the datasheet (e.g. "Port B is a
bi-directional I/O port") and classify each pin as `I`, `O`, or `B`
based on the direction language used:

- A blanket port-level statement ("Port B is bi-directional") applies to
  every pin on that port *unless* a pin-specific exception is stated
  elsewhere in the datasheet — always check for exceptions before
  trusting the port-level statement.
- Dual-function pins (RESET, fuse-configurable I/O, open-drain pins)
  need a human decision, not a silent default. Flag these explicitly
  rather than picking a classification and moving on.

### Step 5 — Cross-check against the pin diagram's arrows (if present)

Some datasheets draw a directional arrow at every pin in the summary Pin
Diagram figure: into the chip for input-only, out of the chip for
output-only, double-headed for bidirectional. Not all datasheets have
this — when it's there, use it as a second, independent check against
your Step 4 classification, and flag any disagreement rather than
silently picking one answer.

**Caveat:** power/supply pins (VDD, VSS, and any internally-generated
supply pin, such as a USB transceiver's regulator output) are often
drawn with a plain single "into chip" arrow purely as a supply-pin
drawing convention — not as a literal signal direction. Don't reclassify
a `V`/`G`/power-type `O` pin as `I` just because of this arrow. The
table's pin-type/buffer-type language stays authoritative for power
pins; the arrow is only a tie-breaker for genuine signal pins.

### Step 6 — Copy alt-function text

Copy the alternate-function list verbatim from the pinout diagram's pin
labels, stripped of surrounding parentheses, and append it after the
pin name as `altText`.

### Step 7 — Sort pins in ascending register order

Sort every pin *except* `VDD`, `SIMCLK`, and `VSS` (which have fixed
positions per Section 2) in ascending alphabetical/register order by pin
name, using natural numeric ordering for any numeric suffix. This
automatically groups pins that share a port register (RA, RB, RC, ...)
together, sorted ascending by pin number within the group. Non-port
utility pins (oscillator, regulator, etc.) simply fall into their
natural alphabetical position in this same sort — they aren't grouped
specially.

### Step 8 — Assemble the file per the layout convention

Place `V <positive supply>` first, then the sorted pins filling the left
column, then (if needed) an `X` spacer and `K SIMCLK` to close out the
left column, then the remaining sorted pins filling the right column,
then `G <ground>` last.

### Step 9 — Set `R` if needed

Add an `R <ohms>` record if the DLL-driven output resistance should
differ from the default of 1 Ω. This is mainly useful for tutorial or
reference files where you want the value to be explicit rather than
implicit.

---

## 4. Worked Example: PIC18F4455/4550 (40-Pin PDIP)

A condensed illustration of the conventions above (full pin list omitted
for brevity — see `PIC18F4x5x.qpindef` for the complete file):

```
D Microchip PIC18F4455/4550 40-pin, USB, Enhanced Flash microcontroller
P PIC18F4455 PIC18F4550

V VDD
* NOTE: pin 32 is a second physical VDD pin (same net as pin 11's VDD).
* No separate record is created for it.

I OSC1 CLKI                     * input-only, no GPIO alt function

B RA0 AN0                       * PORTA ascending: RA0..RA6
...
B RA6 OSC2/CLKO                 * FLAGGED: oscillator-mode dependent

B RB0 AN12/INT0/FLT0/SDI/SDA    * PORTB ascending: RB0..RB7
...
B RC0 T1OSO/T13CKI              * PORTC starts, split by column boundary

X                                * padding: makes the total even
K SIMCLK

B RC1 T1OSI/CCP2/UOE            * PORTC continues
...
I RC4 D-/VM                     * FLAGGED: pin-level exception (input-only)
I RC5 D+/VP                     * FLAGGED: pin-level exception (input-only)
...
B RE0 AN5/CK1SPP                * PORTE ascending: RE0..RE3
...
I RE3 MCLR/VPP                  * FLAGGED: pin-level exception (input-only)

O VUSB                          * FLAGGED: regulator output, not a real VDD

G VSS
* NOTE: pin 31 is a second physical VSS pin (same net as pin 12's VSS).
* No separate record is created for it.
```

Notable decisions from this example:

- **RE3** (MCLR/VPP/RE3) — flagged as a pin-level exception to "PORTE is
  bidirectional"; the table documents RE3 as input-only on this shared
  pin, unlike the rest of PORTE.
- **RA6** (OSC2/CLKO/RA6) — flagged as oscillator-mode dependent; only
  usable as GPIO when the device isn't configured for an external
  crystal on this pin. The pin diagram's double-headed arrow confirmed
  `B` as the right call here.
- **RC4/RC5** (D-/VM, D+/VP) — flagged as pin-level exceptions to
  "PORTC is bidirectional"; both are individually documented (and
  arrow-confirmed) as input-only.
- **VUSB** — an internal regulator output, not a true VDD; classified
  `O`, despite the pin diagram showing the same generic "into chip"
  arrow used for VDD/VSS (a power-pin drawing convention, not a real
  signal direction — see Step 5's caveat).
- **Duplicate VDD/VSS pins** (pins 32 and 31) — documented as comments,
  not given synthetic names like `VDD2`.

---

## 5. Common Pitfalls

- **Pin name case** — pin names in the `.qpindef` file must exactly
  match the names the DLL code and schematic will reference; be
  consistent about case.
- **Treating dual-function pins as ordinary GPIO without flagging them**
  — RESET, open-drain, or fuse-configurable pins need a human decision,
  not a silent default.
- **Forgetting `K SIMCLK`** — every device needs it; it isn't in any
  datasheet.
- **Mismatched variants in one file** — if two part numbers in a family
  have different pin counts or pinouts, they need separate `.qpindef`
  files, even if `P` would otherwise list them together.
- **Assuming port-level "bidirectional" language applies unconditionally**
  — always check for a pin-specific exception before trusting the
  port-level statement.
- **Using `X` for a physical pin** — `X` is layout-only spacing; it does
  not represent a real pin and will not appear in generated code.
- **Inventing a new pin name for a duplicate physical pin** — if a
  second physical pin shares a net with one already defined (extra
  VDD/GND pins are the common case), document it with a comment instead
  of creating `VDD2`/`GND2`/etc.
- **Trusting a pin-diagram arrow over the table for a power pin** — VDD,
  VSS, and internally-generated supply pins are often drawn with a
  generic "into chip" arrow that doesn't reflect true signal direction.
- **Guessing the left/right column split on an odd pin count** — pad
  with one `X` before `SIMCLK` instead of assuming which way QSymGen3
  rounds.
