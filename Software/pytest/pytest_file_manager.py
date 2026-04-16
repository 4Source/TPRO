import pytest
from pytest_embedded_idf.dut import IdfDut

def test_file_manager(dut: IdfDut) -> None:
    # We expect these logs to appear when run_selftest() is called!
    dut.expect("--- Starting FileManager Selftest ---", timeout=15)
    dut.expect("Initializing SD card", timeout=5)
    dut.expect("SDCard mounted at:", timeout=10)
    dut.expect("Saving file:", timeout=5)
    dut.expect("Reading file:", timeout=5)
    dut.expect("Listing directory:", timeout=5)
    dut.expect("Deleting file:", timeout=5)
    dut.expect("--- FileManager Selftest Passed ---", timeout=5)
    print("Test bestanden: FileManager SD Card Operations (Save, Read, List, Delete) funktionieren einwandfrei!")
