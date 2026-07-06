# Prompting Playbook: Generating a `.qpindef` File With Claude

This is a reusable prompt + procedure for getting Claude to turn an MCU
datasheet into a `.qpindef` Pin Definitions file for QSymGen3. Hand this
document to Claude alongside the target datasheet (PDF upload or pasted
text) and it can follow the procedure directly.

This document is distinct from `pindef_from_datasheet_howto.md`, which
teaches a *human* the file format — this one teaches *Claude* the
extraction procedure.

## How to use this playbook

1. Start a new conversation (or continue an existing QSymGen3/QMdbSim2
   one).
2. Upload the datasheet PDF, or paste the relevant pinout/port-description
   sections.
3. Paste the prompt block below, adjusting the device name(s).
4. Review Claude's output against the checklist in Section 3 before
   trusting it — this is a starting draft, not a final answer.

## 1. Prompt block to give Claude

```
I need you to generate a .qpindef Pin Definitions file for QSymGen3 from
the attached datasheet for <DEVICE NAME(S)>.

Format spec:
- Each record is one line: <type> <pinName> [altText]
- Comment lines start with *
- Record types:
  D <description>       -- one line, device family description
  P <part> <part> ...    -- exact part numbers this file covers
  R <ohms>               -- optional, ROUT for output pins, default 1
  I <pin> [alt]           -- input-only pin
  O <pin> [alt]           -- output-only pin
  B <pin> [alt]           -- bidirectional pin
  V <pin>                -- VDD, exactly one
  G <pin>                -- GND, exactly one
  K <pin>                -- SimClock, synthetic, always add exactly one
                             (not in the datasheet -- name it SIMCLK)
  X                      -- spacer, layout only, never a real pin

Procedure:
1. Pull D/P from the title page and ordering/part-number info.
2. Find V and G from the pinout diagram (exactly one of each).
3. Add exactly one K SIMCLK record -- it's synthetic, not from the
   datasheet. Do not go looking for it in the pinout.
4. For every remaining pin, find the datasheet's port-level or
   pin-level description and classify it I/O/B based on the direction
   language used ("input," "output," "bi-directional"). A port-level
   statement applies to the whole port unless a pin-specific exception
   is stated elsewhere -- check for exceptions before trusting it.
5. If the datasheet's Pin Diagram figure draws a directional arrow at
   each pin (into the chip = input-only, out of the chip = output-only,
   double-headed = bidirectional), cross-check every I/O/B
   classification from Step 4 against that arrow. Not all datasheets
   have this -- when present, treat it as a second, independent source
   and flag any disagreement with the table text rather than silently
   picking one. EXCEPTION: pins that are fundamentally power/supply
   pins (VDD, VSS, and any internally-generated supply pin such as a
   USB transceiver regulator output) are often drawn with a plain
   single "into chip" arrow purely as a supply-pin drawing convention,
   not as a literal input/output signal direction -- don't reclassify
   a V/G/O power-type pin as I just because of this arrow; the table's
   pin-type/buffer-type language stays authoritative for power pins.
6. Flag any dual-function or non-obvious pin (RESET, fuse-configurable
   I/O, open-drain) explicitly instead of silently picking a
   classification.
7. Copy alt-function text verbatim from the pinout diagram's pin
   labels, stripped of parentheses, and append after the pin name.
8. If a physical pin shares a net/name with a pin already defined
   elsewhere in the file (e.g. a device with two physical VDD pins, or
   two physical GND pins), do NOT invent a new pin name (VDD2, GND2,
   etc.) for it. Add a comment noting the duplicate pin number and
   which existing record it corresponds to, and do not create a second
   record at all.
9. Sort every pin EXCEPT VDD, SIMCLK, and VSS (which have fixed layout
   positions -- see Step 10) in ascending alphabetical/register order
   by pin name, using natural numeric ordering for any numeric suffix.
   This automatically groups pins that share a port register (RA, RB,
   RC, ...) together, sorted ascending by pin number within the group.
   Non-port utility pins (oscillator, regulator, etc.) simply fall into
   their natural alphabetical position in this same sort -- they are
   not grouped specially.
10. Order pins by symbol layout position. Pin *record order* controls
    left/right column placement in the generated .qsym symbol: the
    first half of records become the left column (top to bottom), the
    second half become the right column (top to bottom). Within that
    constraint:
      - The positive supply pin (VDD/VCC/etc.) is the FIRST record
        overall -- top-left of the symbol.
      - SIMCLK is the LAST record of the left column -- bottom-left.
      - The ground pin (VSS/GND/VEE/etc.) is the LAST record overall --
        bottom-right.
    All other pins (already sorted per Step 9) fill the remaining
    left-column slots first, then the remaining right-column slots, in
    that sorted order.
    If the total record count (including V/G/K) is odd, insert one X
    spacer record immediately before SIMCLK. This brings the total to
    an even number, making the left/right split an exact half-and-half
    with no dependence on how QSymGen3 rounds size/2 -- it guarantees
    SIMCLK lands as the last entry of the left column regardless of
    rounding direction. Do not rely on the rounding assumption instead
    of padding; verify the record count and pad before finalizing.
11. Show me your reasoning for each classification before finalizing,
    especially for anything ambiguous.

Output the file in a code block, followed by your reasoning.
```

## 2. Why the prompt is structured this way

- **Format spec inline, every time.** Don't assume Claude retains the
  format from a prior session — restating it removes ambiguity and
  keeps output consistent across devices and across Claude instances.
- **K SIMCLK called out as synthetic.** Without this note, Claude will
  either omit it (it isn't in any datasheet) or hunt for a clock pin
  that doesn't exist in the sense meant here.
- **Explicit "flag, don't guess" instruction for dual-function pins.**
  This is the single highest-value line in the prompt. Pins like RESET
  or fuse-configurable I/O are exactly where a fully automated pass
  will confidently produce a wrong answer if not told to surface
  uncertainty instead.
- **Asking for reasoning, not just the file.** A `.qpindef` file with
  no rationale is hard to audit. Requiring Claude to explain each
  classification turns the review step (Section 3) into a fast
  pass/fail check per pin instead of a re-derivation from scratch.
- **Comment, don't invent, for duplicate pin names.** Devices with more
  than one physical VDD or GND pin used to get synthetic names like
  `VDD2`. That's gone — a duplicate is now just a comment pointing back
  at the real record, since it isn't a separate net and doesn't need
  its own symbol pin.
- **Ascending register-order sort.** Grouping pins by port (RA, RB,
  RC, ...) and sorting ascending within each group makes the generated
  symbol's pin layout predictable and easy to eyeball against the
  datasheet, instead of mirroring an arbitrary physical pin-number
  order.
- **Pin-diagram arrows as a second opinion, not a first one.** Some
  datasheets draw a directional arrow at every pin in the summary Pin
  Diagram figure. When present, it's a fast, independent cross-check
  against the port-level table language -- but it isn't always
  reliable for power pins, which tend to get a generic "into chip"
  arrow regardless of their real electrical role (e.g. a USB regulator
  output pin can still be drawn the same way as VDD). Table text
  remains authoritative for power-type pins; the arrow is only a
  tie-breaker/sanity-check for genuine signal pins.

## 3. Review Checklist (do this before trusting the output)

- [ ] `D`/`P` match the datasheet title page and ordering info exactly.
- [ ] Exactly one `V` and one `G` record present.
- [ ] Exactly one `K SIMCLK` record present, and it wasn't "found" in
      the datasheet (it shouldn't be — it's synthetic).
- [ ] Every GPIO pin's `I`/`O`/`B` classification traces back to actual
      datasheet direction language, not an assumption.
- [ ] If the datasheet's Pin Diagram has directional arrows, every
      signal-pin classification was cross-checked against its arrow,
      and any disagreement was flagged rather than silently resolved.
      Power/supply pins (V/G, and internally-generated supply pins) are
      exempt from this check -- their arrow is often just a drawing
      convention, not a real signal direction.
- [ ] Any dual-function pin (RESET, open-drain, fuse-configurable) is
      explicitly flagged, not silently classified.
- [ ] `altText` matches the pinout diagram's alt-function list verbatim
      (parentheses stripped).
- [ ] Positive supply pin is the first record (top-left); SIMCLK is the
      last record of the left column (bottom-left); ground pin is the
      last record overall (bottom-right).
- [ ] All other pins are sorted in ascending alphabetical/register
      order (grouped by port, ascending within each port), not
      datasheet physical pin-number order.
- [ ] If the total record count is odd, an X spacer was inserted before
      SIMCLK to force an even, unambiguous left/right split.
- [ ] If the family covers multiple part numbers with different pin
      counts or pinouts, they've been split into separate files rather
      than combined under one `P` record.
- [ ] Any pin sharing a name/net with a pin already defined elsewhere
      in the file (e.g. a second physical VDD or GND pin) has a comment
      noting the duplicate — no synthetic name like `VDD2` was created
      and no second record exists for it.

## 4. Known open items (unresolved as of the last extraction session)

- Open-drain/open-collector as a first-class pin type (needed for pins
  like RESET) does not exist yet — it would require a new
  `GPIO_IMPL.qsch` variant, which is a nontrivial amount of additional
  code. Until it exists, dual-function pins are approximated as `B`
  and flagged for manual review.
- Devices with more than one physical VDD or GND pin: resolved as of
  this revision — duplicates get a comment, not a synthetic record
  (see Step 7). No longer an open item.
