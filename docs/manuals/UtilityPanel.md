# UtilityPanel — Operating Manual

*A resizable blanking panel for the VCV Rack environment — part of the Shapetaker series.*

## Description

The **UtilityPanel** is a structural module designed to elegantly fill empty spaces within your system. Rather than relying on a collection of fixed-width blanking plates, the operator can deploy a single UtilityPanel and seamlessly adjust its width to occupy any gap from a narrow 2 HP up to a substantial 64 HP. 

## Initial Operation

Upon initial installation into the rack, the UtilityPanel is automatically configured to a minimal width of 2 HP. During the first moment of placement, the module employs an intelligent auto-fitting topology. It scans the horizontal row for adjacent modules and will automatically expand its chassis to perfectly fill the surrounding gap. 

## Architecture

### Resizing Handles
The left and right edges of the panel are equipped with discrete, opaque resizing handles. The operator may click and drag on either edge of the panel to manually expand or contract the module's width in 1 HP increments.

Double-click anywhere on the panel body or either resize edge to repeat the automatic gap-fitting operation. This uses the same surrounding-slot measurement performed when a new UtilityPanel is first added to the rack.

### Aesthetic Design
The faceplate features a textured, layered background designed to blend sympathetically with both vintage and modern instrumentation, anchored by centering screws that dynamically reposition themselves as the panel width is adjusted. 

## Calibration and Advanced Settings

The UtilityPanel provides the following option within its context menu (accessed via right-click):

* **Fit to surrounding gap**: Manually triggers the auto-fitting algorithm. This is useful if the arrangement of neighboring modules has changed and you wish the panel to instantly resize to fill the new dimensions of the gap.

## Operating Considerations

* **Dimensional Limits**: The physical architecture of the UtilityPanel dictates a minimum width of 2 HP and a maximum width of 64 HP.
* **Auto-Fit Constraints**: The automatic gap-filling algorithm evaluates space within a given vertical row tolerance. If the available space exceeds the 64 HP maximum limit, the panel will expand to its maximum width. 
