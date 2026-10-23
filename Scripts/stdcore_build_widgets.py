"""Testbed only: rebuilds the layout of the Std Core Widget Blueprints through the editor's MCP server.

Each Widget Blueprint's tree is removed and rebuilt from the definitions below, then compiled and saved. The widget
names required by the C++ classes (BindWidget) are kept; colors and fonts are not set here, they come from the UI
theme (DA_UITheme) at Pre Construct. Python inside the editor cannot edit widget trees, hence the MCP UMG toolset.

Requires the testbed editor open with the MCP server on 127.0.0.1:8003.
    python stdcore_build_widgets.py [WBP_Name ...]        (default: all of them)
"""
import json
import sys

import mcp_client as mcp

UMG = "UMGToolSet.UMGToolSet"
OBJ = "editor_toolset.toolsets.object.ObjectTools"
ASSET = "editor_toolset.toolsets.asset.AssetTools"
FOLDER = "/StdCore/UI"


# ---------------------------------------------------------------------------------------------------------------------
# Tree definition helpers
# ---------------------------------------------------------------------------------------------------------------------

def W(name, cls, props=None, slot=None, children=None, bound=False):
    """A widget: UMG class name, its properties, its slot properties, its children; bound = BindWidget in C++."""
    return {"name": name, "cls": cls, "props": props or {}, "slot": slot or {}, "children": children or [], "bound": bound}


def pad(left=0, top=0, right=0, bottom=0):
    return {"Padding": {"left": left, "top": top, "right": right, "bottom": bottom}}


def fill(value=1.0, **extra):
    return dict({"Size": {"sizeRule": "Fill", "value": value}}, **extra)


# Fallback look when no theme is set (Project Settings > Std Core > Theme empty): dark layers, dark button labels on
# the default light buttons. The theme overrides all of them.
BACKDROP = {"BrushColor": {"r": 0, "g": 0, "b": 0, "a": 0.5}}
PANEL = {"BrushColor": {"r": 0.01, "g": 0.012, "b": 0.016, "a": 0.92}}
DARK_LABEL = {"specifiedColor": {"r": 0.02, "g": 0.02, "b": 0.02, "a": 1}, "colorUseRule": "UseColor_Specified"}


def text(name, value, bound=False, slot=None, **props):
    return W(name, "TextBlock", dict({"Text": value}, **props), slot, bound=bound)


def button(name, label_name, label, bound_label=False, slot=None):
    return W(name, "Button", slot=slot, bound=True,
             children=[text(label_name, label, bound=bound_label, Justification="Center", ColorAndOpacity=DARK_LABEL)])


def sized(name, child, width=None, height=None, slot=None):
    props = {}
    if width:
        props.update({"bOverride_WidthOverride": True, "WidthOverride": width})
    if height:
        props.update({"bOverride_HeightOverride": True, "HeightOverride": height})
    return W(name, "SizeBox", props, slot, children=[child])


def centered_panel(width, height, children):
    """Dimmed full-screen backdrop, a rounded panel in the middle, a vertical box of the given inner size."""
    return W("Border_Background", "Border", dict(BACKDROP, HorizontalAlignment="HAlign_Center", VerticalAlignment="VAlign_Center"),
             children=[W("Border_Panel", "Border", dict(PANEL, Padding={"left": 32, "top": 32, "right": 32, "bottom": 32}),
                         children=[sized("SizeBox_Main", W("Box_Main", "VerticalBox", children=children), width, height)])])


def footer(*children):
    return W("Box_Footer", "HorizontalBox", slot=pad(top=24), children=list(children))


def spacer(name, slot=None):
    return W(name, "Spacer", slot=slot)


def setting_row(row_name, label_name, label, control, value=None):
    """Label (40%), control, optional value text: one line of the Audio / Video tab."""
    children = [text(label_name, label, slot=fill(0.4, VerticalAlignment="VAlign_Center")),
                dict(control, slot=fill(0.5 if value else 0.6, VerticalAlignment="VAlign_Center"))]
    if value:
        children.append(text(value, "100%", bound=True, Justification="Right",
                             slot=fill(0.1, VerticalAlignment="VAlign_Center")))
    return W(row_name, "HorizontalBox", slot=pad(bottom=16), children=children)


MENU_BUTTON = pad(bottom=12)

# Left and right change the value as soon as the slider has the focus; up and down move to the next row.
SLIDER = {"RequiresControllerLock": False}

# ---------------------------------------------------------------------------------------------------------------------
# Layouts (reference resolution 1920 x 1080)
# ---------------------------------------------------------------------------------------------------------------------

TREES = {
    "WBP_KeyBindingRow": ("/Script/StdCore.StdKeyBindingRowWidget", sized("SizeBox_Root", W("Box_Root", "HorizontalBox", children=[
        text("Text_ActionName", "Action", bound=True, slot=fill(VerticalAlignment="VAlign_Center")),
        sized("SizeBox_Key", W("KeySelector", "InputKeySelector", bound=True), width=220,
              slot={"VerticalAlignment": "VAlign_Center"}),
    ]), height=56)),

    "WBP_Settings": ("/Script/StdCore.StdSettingsWidget", centered_panel(896, 616, [
        W("Box_Header", "HorizontalBox", slot=pad(bottom=24), children=[
            text("Text_Title", "Settings", slot=fill(VerticalAlignment="VAlign_Center")),
            button("Button_TabAudioVideo", "Text_TabAudioVideo", "Audio / Video",
                   slot=dict(pad(left=8), VerticalAlignment="VAlign_Center")),
            button("Button_TabControls", "Text_TabControls", "Controls",
                   slot=dict(pad(left=8), VerticalAlignment="VAlign_Center")),
        ]),
        W("Switcher_Tabs", "WidgetSwitcher", slot=fill(), bound=True, children=[
            W("Box_AudioVideo", "VerticalBox", children=[
                setting_row("Row_MasterVolume", "Text_MasterLabel", "Master volume",
                            W("Slider_MasterVolume", "Slider", SLIDER, bound=True), "Text_MasterVolumeValue"),
                setting_row("Row_MusicVolume", "Text_MusicLabel", "Music volume",
                            W("Slider_MusicVolume", "Slider", SLIDER, bound=True), "Text_MusicVolumeValue"),
                setting_row("Row_SfxVolume", "Text_SfxLabel", "Effects volume",
                            W("Slider_SfxVolume", "Slider", SLIDER, bound=True), "Text_SfxVolumeValue"),
                setting_row("Row_WindowMode", "Text_WindowModeLabel", "Window mode",
                            button("Button_WindowMode", "Text_WindowMode", "Fullscreen", bound_label=True)),
                setting_row("Row_Quality", "Text_QualityLabel", "Quality",
                            button("Button_Quality", "Text_Quality", "High", bound_label=True)),
            ]),
            W("Box_Controls", "VerticalBox", children=[
                W("ScrollBox_KeyBindings", "ScrollBox", {"ScrollWhenFocusChanges": "AnimatedScroll"}, slot=fill(),
                  children=[W("Box_KeyBindings", "VerticalBox", bound=True)]),
                text("Text_KeyBindingMessage", "", bound=True, slot=pad(top=12), AutoWrapText=True),
                # Right-aligned under the key selectors, so Down from the last row reaches Reset before Back.
                W("Box_ControlsFooter", "HorizontalBox", slot=pad(top=12), children=[
                    spacer("Spacer_ControlsFooter", slot=fill()),
                    sized("SizeBox_Reset", button("Button_ResetDefaults", "Text_ResetDefaults", "Reset to defaults"), width=260),
                ]),
            ]),
        ]),
        footer(spacer("Spacer_Footer", slot=fill()),
               sized("SizeBox_Back", button("Button_Back", "Text_Back", "Back"), width=200)),
    ])),

    "WBP_Credits": ("/Script/StdCore.StdCreditsWidget", centered_panel(656, 656, [
        text("Text_Title", "Credits", Justification="Center", slot=pad(bottom=16)),
        W("ScrollBox_Credits", "ScrollBox", slot=fill(), bound=True, children=[
            text("Text_Credits", "A game by\nYour Name\n\nMade with Unreal Engine", bound=True,
                 Justification="Center", AutoWrapText=True)]),
        footer(spacer("Spacer_Footer", slot=fill()),
               sized("SizeBox_Back", button("Button_Back", "Text_Back", "Back"), width=200),
               spacer("Spacer_FooterEnd", slot=fill())),
    ])),

    "WBP_PauseMenu": ("/Script/StdCore.StdPauseMenuWidget", centered_panel(376, None, [
        text("Text_Title", "Paused", Justification="Center", slot=pad(bottom=24)),
        button("Button_Resume", "Text_Resume", "Resume", slot=MENU_BUTTON),
        button("Button_Settings", "Text_Settings", "Settings", slot=MENU_BUTTON),
        button("Button_MainMenu", "Text_MainMenu", "Main menu", slot=MENU_BUTTON),
        button("Button_Quit", "Text_Quit", "Quit"),
    ])),

    "WBP_MainMenu": ("/Script/StdCore.StdMainMenuWidget", W("Border_Background", "Border",
        dict(BACKDROP, HorizontalAlignment="HAlign_Fill", VerticalAlignment="VAlign_Fill"), children=[
            W("Overlay_Root", "Overlay", children=[
                sized("SizeBox_Column", W("Box_Main", "VerticalBox", children=[
                    text("Text_Title", "Game Title", slot=pad(bottom=48)),
                    button("Button_Play", "Text_Play", "Play", slot=MENU_BUTTON),
                    button("Button_Settings", "Text_Settings", "Settings", slot=MENU_BUTTON),
                    button("Button_Credits", "Text_CreditsLabel", "Credits", slot=MENU_BUTTON),
                    button("Button_Quit", "Text_Quit", "Quit"),
                ]), width=420, slot=dict(pad(left=160), HorizontalAlignment="HAlign_Left", VerticalAlignment="VAlign_Center")),
                text("Text_Version", "v1.0", slot=dict(pad(right=24, bottom=24), HorizontalAlignment="HAlign_Right",
                                                      VerticalAlignment="VAlign_Bottom")),
            ]),
        ])),
}


# ---------------------------------------------------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------------------------------------------------

def set_props(ref, values):
    if values and not mcp.result(mcp.call(OBJ, "set_properties", {"instance": {"refPath": ref}, "values": json.dumps(values)})):
        raise RuntimeError("set_properties failed on {}: {}".format(ref, values))


def build(name):
    parent_class, root = TREES[name]
    bp_ref = {"refPath": FOLDER + "/" + name + "." + name}

    if not mcp.result(mcp.call(ASSET, "exists", {"path": FOLDER + "/" + name})):
        mcp.result(mcp.call(UMG, "CreateWidgetBlueprint", {"folderPath": FOLDER, "assetName": name, "parentClass": {"refPath": parent_class}}))
        print(name, "created")

    # BindWidget members missing from the tree are listed with "widget": "None"; only real widgets have a parent field.
    for widget in mcp.result(mcp.call(UMG, "GetWidgets", {"widgetBlueprint": bp_ref}))["widgets"]:
        if isinstance(widget.get("widget"), dict) and widget["parent"] == "None":
            mcp.result(mcp.call(UMG, "RemoveWidget", {"widgetBlueprint": bp_ref, "widget": widget["widget"]}))

    count = 0

    def add(node, parent_ref):
        nonlocal count
        args = {"widgetBlueprint": bp_ref, "widgetClass": {"refPath": "/Script/UMG." + node["cls"]}, "widgetDisplayName": node["name"]}
        if parent_ref:
            args["parentWidget"] = {"refPath": parent_ref}
        info = mcp.result(mcp.call(UMG, "AddWidget", args))
        count += 1
        widget_ref = info["widget"]["refPath"]
        if node["bound"]:
            mcp.result(mcp.call(UMG, "ToggleWidgetAsVariable", {"widgetBlueprint": bp_ref, "widget": {"refPath": widget_ref}, "bIsVariable": True}))
        set_props(widget_ref, node["props"])
        if node["slot"] and isinstance(info.get("slot"), dict):
            set_props(info["slot"]["refPath"], node["slot"])
        for child in node["children"]:
            add(child, widget_ref)

    add(root, None)
    compiled = mcp.text_of(mcp.call(UMG, "CompileWidgetBlueprint", {"widgetBlueprint": bp_ref}))
    saved = mcp.text_of(mcp.call(ASSET, "save_assets", {"asset_paths": [FOLDER + "/" + name]}))
    print("{}: {} widgets, compile {}, save {}".format(name, count, compiled, saved))
    if '"returnValue":true' not in compiled.replace(" ", "") or '"returnValue":true' not in saved.replace(" ", ""):
        raise RuntimeError(name + " did not compile or save")


if __name__ == "__main__":
    mcp.init()
    for wbp in sys.argv[1:] or list(TREES):
        build(wbp)
