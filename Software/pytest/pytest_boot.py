import pytest
from pytest_embedded_idf.dut import IdfDut

def test_boot(dut: IdfDut) -> None:
    dut.expect("LED Wall startup", timeout=10) # Wird geprintet am Anfang der app_main.
    print("Test bestanden: ESP hat gebootet!")