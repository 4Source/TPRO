import pytest
import os

@pytest.fixture(scope='session')
def session_tempdir():
    path = os.path.join(os.getcwd(), 'pytest_embedded_files')
    os.makedirs(path, exist_ok=True)
    return path

@pytest.fixture(scope='session')
def cache_dir(session_tempdir):
    path = os.path.join(session_tempdir, 'pytest-embedded-cache')
    os.makedirs(path, exist_ok=True)
    return path