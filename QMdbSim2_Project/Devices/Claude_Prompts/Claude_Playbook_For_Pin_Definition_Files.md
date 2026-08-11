# Prompting Playbook: Generating a `.qpindef` File With Claude

This is a reusable prompt + procedure for getting Claude to turn an MCU
datasheet into a `.qpindef` Pin Definitions file for QSymGen3. Hand this
document to Claude alongside the target datasheet (PDF upload or pasted
text) and it can follow the procedure directly.

This document is distinct from `Creating_Pin_Definition_Files.md`, which
teaches a *human* the file format — this one teaches *Claude* the
extraction procedure.

## How to use this playbook

1. Start a new conversation (or continue an existing QSymGen3/QMdbSim2
   one).
2. **Prefer uploading the datasheet PDF directly** over giving Claude a
   URL to fetch. Vendor sites (Microchip in particular) commonly block
   automated fetching entirely, and Claude falling back to reconstructing
   a pinout from web-search snippets is real, working, but strictly
   lower-confidence than reading the actual PDF's text layer — search
   snippets can scramble multi-column pin diagrams, and no single
   snippet usually contains a whole pinout table. If a direct fetch is
   blocked and you don't have the PDF handy, it's still possible to
   proceed on search snippets, but expect (and ask Claude to include) an
   explicit sourcing-reliability caveat in the resulting file.
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

If given a PDF: extract its text layer directly (e.g. `pdftotext -layout`)
rather than reading page images visually where avoidable -- it's faster
and more reliable. Search the extracted text for the actual pinout-table
heading (commonly something like "TABLE 1-1", "PINOUT I/O DESCRIPTIONS",
or "Pin Diagram") rather than guessing a page number. If no PDF is
available and you're working from web search instead, cross-check pinout
details across at least two independent sources before trusting them,
and say explicitly in your final reasoning that the file was built from
search snippets rather than a primary-source PDF.

Procedure:
1. Pull D from the title page. For P, don't stop at whatever part numbers
   appear in a feature-comparison table -- check the datasheet's Product
   Identification / Ordering Information section, which lays out the
   complete part-number construction (temperature grade, package,
   low-voltage/wide-voltage variants, etc.) and is the authoritative
   source for every pin-identical sibling part, including ones a
   feature-comparison table might not distinguish.
2. Find V and G from the pinout diagram (exactly one of each).
3. Add exactly one K SIMCLK record -- it's synthetic, not from the
   datasheet. Do not go looking for it in the pinout.
4. Identify any pin that is a genuinely separate power rail (e.g. a
   distinct analog AVDD/AVSS) or an internal regulator support pin
   (e.g. VCAP) -- something that is not the same net as VDD/VSS, is not
   GPIO, and doesn't fit I/O/B/V/G cleanly. The established convention
   for these is to record them as I (closest available fit) and flag
   the decision explicitly in a comment explaining what the pin actually
   is and why I was chosen despite the mismatch. Do not invent a new
   record type or silently pick a category without that comment. This
   is different from a duplicate physical VDD/VSS pin (step 2/8 below,
   same net, no separate record at all) -- these are different nets that
   the format has no dedicated category for.
5. For every remaining pin, find the datasheet's port-level or
   pin-level description and classify it I/O/B based on the direction
   language used ("input," "output," "bi-directional"). A port-level
   statement applies to the whole port unless a pin-specific exception
   is stated elsewhere -- check for exceptions before trusting it.
   If the device uses Peripheral Pin Select (PPS) or similar runtime-
   remappable digital peripheral assignment (look for an RPnn-style
   notation alongside the pin name), that does NOT by itself make a pin
   ambiguous or require a flag -- the underlying GPIO bit is
   bidirectional regardless of what's remapped onto it. Only flag a
   PPS-capable pin if something else about it is non-obvious.
6. If the datasheet's Pin Diagram figure draws a directional arrow at
   each pin (into the chip = input-only, out of the chip = output-only,
   double-headed = bidirectional), cross-check every I/O/B
   classification from step 5 against that arrow. Not all datasheets
   have this -- when present, treat it as a second, independent source
   and flag any disagreement with the table text rather than silently
   picking one. EXCEPTION: pins that are fundamentally power/supply
   pins (VDD, VSS, and any internally-generated supply pin such as a
   USB transceiver regulator output) are often drawn with a plain
   single "into chip" arrow purely as a supply-pin drawing convention,
   not as a literal input/output signal direction -- don't reclassify
   a V/G/O power-type pin as I just because of this arrow; the table's
   pin-type/buffer-type language stays authoritative for power pins.
7. Flag any dual-function or non-obvious pin (RESET, fuse-configurable
   I/O, open-drain) explicitly instead of silently picking a
   classification. Distinguish this from a pin where a special function
   (e.g. Reset) is simply dedicated with no GPIO alternate function
   listed anywhere -- that's not ambiguous and doesn't need a flag, just
   a plain I/O/B record. Also note, as a flagged comment, any
   pin-specific footnote in the pinout description table even if it
   doesn't change the pin's classification -- e.g. a documented
   power-up glitch or an unusual loading/usage caveat. These matter to
   anyone using the pin later even though they aren't a classification
   question.
8. Copy alt-function text verbatim from the pinout diagram's pin
   labels, stripped of parentheses, and append after the pin name. On
   PPS-capable devices this list is often longer (5-6 alternate names
   per pin is common) -- copy all of it, same as any other pin.
9. If a physical pin shares a net/name with a pin already defined
   elsewhere in the file (e.g. a device with two physical VDD pins, or
   two physical GND pins), do NOT invent a new pin name (VDD2, GND2,
   etc.) for it. Add a comment noting the duplicate pin number and
   which existing record it corresponds to, and do not create a second
   record at all.
10. Sort every pin EXCEPT VDD, SIMCLK, and VSS (which have fixed layout
    positions -- see step 11) in ascending alphabetical/register order
    by pin name, using natural numeric ordering for any numeric suffix.
    This automatically groups pins that share a port register (RA, RB,
    RC, ...) together, sorted ascending by pin number within the group.
    Non-port utility pins (oscillator, regulator, etc.), including any
    pin classified I under step 4's workaround, simply fall into their
    natural alphabetical position in this same sort -- they are not
    grouped specially.
11. Order pins by symbol layout position. Pin *record order* controls
    left/right column placement in the generated .qsym symbol: the
    first half of records become the left column (top to bottom), the
    second half become the right column (top to bottom). Within that
    constraint:
      - The positive supply pin (VDD/VCC/etc.) is the FIRST record
        overall -- top-left of the symbol.
      - SIMCLK is the LAST record of the left column -- bottom-left.
      - The ground pin (VSS/GND/VEE/etc.) is the LAST record overall --
        bottom-right.
    All other pins (already sorted per step 10) fill the remaining
    left-column slots first, then the remaining right-column slots, in
    that sorted order.
    If the total record count (including V/G/K) is odd, insert one X
    spacer record immediately before SIMCLK. This brings the total to
    an even number, making the left/right split an exact half-and-half
    with no dependence on how QSymGen3 rounds size/2 -- it guarantees
    SIMCLK lands as the last entry of the left column regardless of
    rounding direction. Do not rely on the rounding assumption instead
    of padding; verify the record count and pad before finalizing.
12. If the datasheet covers more than one physical package (PDIP, SOIC,
    SSOP, QFN, etc.), a separate .qpindef file is very rarely needed --
    the format doesn't encode physical pin numbers, only signal names,
    alt-function text, and classification, so pin-identical packages
    share one file. What differs between packages goes in comments, not
    records: the VDD/VSS pin count per package (step 9's duplicate-pin
    handling, extended per package), any recommendation to tie an
    exposed thermal pad to ground, and the existence of No-Connect (NC)
    pins on larger packages. NC pins and exposed pads never get their
    own record. Only produce genuinely separate files if packages differ
    in actual signal set, not just pin count.
13. Show me your reasoning for each classification before finalizing,
    especially for anything ambiguous, and state plainly whether this
    file was built from a primary-source PDF or reconstructed from web
    search snippets.

Output the file in a code block, followed by your reasoning.
```

## 2. Why the prompt is structured this way

- **Format spec inline, every time.** Don't assume Claude retains the
  format from a prior session — restating it removes ambiguity and
  keeps output consistent across devices and across Claude instances.
- **PDF-sourcing guidance up front.** Vendor documentation sites
  frequently block automated fetching outright. Claude *can* fall back
  to reconstructing a pinout from search-engine-indexed snippets, and
  that path has worked, but it's real extra effort (many targeted
  queries) and lower confidence than an uploaded PDF's actual text
  layer — snippets can scramble multi-column pin diagrams during
  indexing, and no single snippet typically contains a whole table.
  Naming this upfront sets the right expectation and gets an explicit
  reliability caveat into the output when the fallback path is used,
  rather than presenting a reconstructed file with the same confidence
  as one read directly from the source.
- **K SIMCLK called out as synthetic.** Without this note, Claude will
  either omit it (it isn't in any datasheet) or hunt for a clock pin
  that doesn't exist in the sense meant here.
- **Non-VDD/GND power pins get an explicit workaround, not silence.**
  Mixed-signal and DSC-class devices commonly expose a separate analog
  supply rail or an internal regulator's support pin — neither VDD/VSS,
  neither GPIO. The format has no dedicated category for either. The
  established convention is `I`, flagged with a comment explaining the
  mismatch, so a future reader knows it's a deliberate workaround for a
  real format gap rather than a considered classification.
- **PPS-remappable functions are explicitly told not to trigger a
  flag.** Without this, a device with heavy peripheral-pin-select usage
  would generate a dual-function flag on nearly every pin, burying the
  flags that actually matter (a genuinely fixed dual-function pin like
  a shared Reset) under noise from a pattern that isn't actually
  ambiguous.
- **Explicit "flag, don't guess" instruction for dual-function pins.**
  This is one of the highest-value lines in the prompt. Pins like RESET
  or fuse-configurable I/O are exactly where a fully automated pass
  will confidently produce a wrong answer if not told to surface
  uncertainty instead. Carrying forward pin-specific footnotes (even
  ones that don't change classification) is the same instinct applied
  more broadly — the goal is nothing operationally important gets
  silently dropped just because it didn't affect an I/O/B decision.
- **Multi-package handling folded into the main procedure.** Before
  this was added explicitly, there was a real risk of either producing
  redundant near-duplicate files per package, or missing package-
  specific details (VDD/VSS counts, exposed pads, NC pins) that do
  matter even though they don't change any record.
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

- [ ] `D`/`P` match the datasheet title page. `P` was checked against
      the Product Identification/Ordering Information section, not just
      whatever feature-comparison table happened to be visible.
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
      explicitly flagged, not silently classified. A dedicated
      special-function pin with no GPIO alternate function is correctly
      *not* flagged (it isn't ambiguous).
- [ ] Any non-VDD/GND power-type pin (separate analog rail, internal
      regulator support pin) is recorded as `I` with an explicit comment
      explaining what it actually is.
- [ ] Any pin-specific footnote from the pinout table (electrical
      quirks, usage caveats) that doesn't affect classification is still
      carried forward as a flagged comment, not dropped.
- [ ] PPS-remappable pins were not individually flagged merely for being
      remappable.
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
      than combined under one `P` record. Multiple *packages* with the
      same signal set are correctly kept in one file, with only
      per-package VDD/VSS counts, exposed-pad notes, and NC-pin notes
      added as comments.
- [ ] Any pin sharing a name/net with a pin already defined elsewhere
      in the file (e.g. a second physical VDD or GND pin) has a comment
      noting the duplicate — no synthetic name like `VDD2` was created
      and no second record exists for it.
- [ ] If built from web search rather than an uploaded PDF, the file (or
      the accompanying reasoning) says so explicitly, and pinout details
      were cross-checked against at least two independent sources.

## 4. Known open items (unresolved as of the last extraction session)

- **Open-drain/open-collector as a first-class pin type** (needed for
  pins like RESET) does not exist yet — it would require a new
  `GPIO_IMPL.qsch` variant, which is a nontrivial amount of additional
  code. Until it exists, dual-function pins are approximated as `B`
  and flagged for manual review. Unlike the item below, there is
  currently no workaround at all for this one, not even an imprecise
  one.
- **Non-VDD/GND power-type pins** (separate analog rails like
  AVDD/AVSS, internal regulator support pins like VCAP) have a
  documented workaround (record as `I`, flag with a comment — see
  Section 1 of `Creating_Pin_Definition_Files.md`) but no dedicated
  format support. Worth a real fix eventually (a `P`-for-power-type
  record, or similar) if these keep coming up across more devices; for
  now the workaround is consistent and good enough to proceed on.
- Devices with more than one physical VDD or GND pin: resolved as of
  this revision — duplicates get a comment, not a synthetic record
  (see Step 9). No longer an open item.
- Devices with more than one physical *package*: resolved as of this
  revision — one file covers every pin-identical package, with
  per-package differences (VDD/VSS counts, exposed pads, NC pins)
  captured as comments (see Section 1 and Step 12). No longer an open
  item.
