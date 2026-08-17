Import("env")

from pathlib import Path


driver = (
    Path(env.subst("$PROJECT_LIBDEPS_DIR"))
    / env.subst("$PIOENV")
    / "GxEPD2"
    / "src"
    / "epd3c"
    / "GxEPD2_750c_GDEW075Z08.cpp"
)

marker = "// FPC-8612 booster soft-start"
needle = """  _writeData (0x3f);   // VDL=-15V
  _writeCommand(0x00); // PANEL SETTING"""
replacement = """  _writeData (0x3f);   // VDL=-15V
  // FPC-8612 booster soft-start: improve VGH/VGL on UC8179 clone panels.
  _writeCommand(0x06); // BOOSTER SOFT START
  _writeData(0x17);
  _writeData(0x27);
  _writeData(0x37);
  _writeData(0x17);
  _writeCommand(0x00); // PANEL SETTING"""

if driver.exists():
    source = driver.read_text(encoding="utf-8")
    if marker not in source:
        if needle not in source:
            raise RuntimeError("FPC-8612 patch target was not found in GxEPD2")
        driver.write_text(source.replace(needle, replacement, 1), encoding="utf-8")
        print("Applied FPC-8612 booster patch to GxEPD2")
