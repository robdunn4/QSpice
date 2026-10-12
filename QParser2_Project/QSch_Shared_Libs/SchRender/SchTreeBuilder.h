#pragma once
#include <QGraphicsScene>
#include <QList>

#include "ItemTree.h"

namespace sch::render {

// Walks a parsed ItemTree and builds the corresponding QSpiceGraphics
// wrapper items into scene.
//
// Design note: a COMP node's nested SYM child contributes no geometry of
// its own (ItemSym only carries a name reference) -- it exists purely to
// group the symbol's drawable elements. Rather than create a matching
// (otherwise empty) SymSchItem wrapper, this flattens the SYM child's
// children directly onto the SchItemComp, one level shallower than the
// model tree.
//
// All element types that appear in a schematic now have a wrapper
// (RECT/LINE/WIRE/ELLIPSE/ARC3P/ZIGZAG/PIN/TEXT/NET, plus COMP as the
// positioning container). ZIGZAG's waveform, PIN's label, and TEXT/NET's
// rotation are known approximations/stubs -- see their respective
// wrapper classes.
//
// NOTE: this only builds the visual scene -- it does not retain the
// ItemTree itself anywhere. Once Save needs to write edits back out,
// something (likely SchView) will need to own the ItemTree for the
// lifetime of the document, not just use it transiently here.
namespace SchTreeBuilder {

// Builds wrapper items for the tree's top-level elements WITHOUT adding
// them to a scene, each linked to the node it came from. Paste uses
// this: pasted content has to be positioned against the cursor before
// it becomes part of the document.
QList<QGraphicsItem *> buildTopLevelItems(const ItemTree &tree);

void populateScene(const ItemTree &tree, QGraphicsScene *scene);

} // namespace SchTreeBuilder

} // namespace sch::render
