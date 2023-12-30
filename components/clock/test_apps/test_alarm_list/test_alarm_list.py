# SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: CC0-1.0

import pytest
from pytest_embedded import Dut

def test_alarm_list(dut: Dut) -> None:
    dut.run_all_single_board_cases(timeout=120)

