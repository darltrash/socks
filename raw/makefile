ifneq ($(shell flatpak list | grep org.blender.Blender),)
    BLENDER ?= flatpak run org.blender.Blender
else
    BLENDER ?= blender
endif

# Source .blend files
BLENDRR_SRC_FILES = $(wildcard *.blend)

EXM_DIR = ../package/assets
EXM_FILES = $(patsubst %.blend, $(EXM_DIR)/%.exm, $(BLENDRR_SRC_FILES))

blender: $(EXM_FILES)

$(EXM_DIR)/%.exm: %.blend
	@mkdir -p $(EXM_DIR)
	$(BLENDER) $(abspath $<) --background --python-expr "import bpy, sys; bpy.ops.wm.open_mainfile(filepath='$(abspath $<)'); bpy.ops.export.exm(filepath='$(abspath $@)')"
