import os
from typing import List

from fboss.platform.bsp_tests.test_runner import FpgaSpec, TestBase

from fboss.platform.bsp_tests.utils.cdev_types import I2CDevice

from fboss.platform.bsp_tests.utils.cdev_utils import delete_device, make_cdev_path
from fboss.platform.bsp_tests.utils.cmd_utils import run_cmd
from fboss.platform.bsp_tests.utils.i2c_utils import (
    create_i2c_adapter,
    create_i2c_device,
    detect_i2c_device,
    parse_i2cdump_data,
)


class TestI2c(TestBase):
    fpgas: List[FpgaSpec] = []

    @classmethod
    def setup_class(cls):
        super().setup_class()
        cls.fpgas = cls.config.fpgas

    def setup_method(self):
        self.load_kmods()

    def test_cdev_is_created(self) -> None:
        for fpga in self.fpgas:
            path = make_cdev_path(fpga)
            assert os.path.exists(path)

    def test_i2c_adapter_creates_busses(self) -> None:
        for fpga in self.fpgas:
            for adapter in fpga.i2cAdapters:
                # Creates adapter, checks expected number of busses created
                newAdapters, _ = create_i2c_adapter(fpga, adapter)

                # Check each bus has a unique name
                names = set()
                for a in newAdapters:
                    names.add(a.name)
                assert len(names) == len(newAdapters)
                delete_device(fpga, adapter.auxDevice)

    def test_i2c_adapter_devices_exist(self) -> None:
        """
        Tests that each expected device is detectable
        """

        for fpga in self.fpgas:
            for adapter in fpga.i2cAdapters:
                assert adapter.auxDevice.i2cInfo
                # record the current existing busses
                newAdapters, adapterBaseBusNum = create_i2c_adapter(fpga, adapter)

                for device in adapter.i2cDevices:
                    print(
                        f"\nChecking for device {device.address} on bus {adapterBaseBusNum + device.channel}"
                    )
                    assert detect_i2c_device(
                        adapterBaseBusNum + device.channel, device.address
                    )
                delete_device(fpga, adapter.auxDevice)

    def test_i2c_bus_with_devices_can_be_unloaded(self) -> None:
        """
        Create bus, create devices on that bus, ensure that the bus
        driver can be unloaded successfully.
        """
        for fpga in self.fpgas[0:1]:
            for adapter in reversed(fpga.i2cAdapters):
                self.load_kmods()
                _, adapterBaseBusNum = create_i2c_adapter(fpga, adapter)
                for device in adapter.i2cDevices:
                    assert detect_i2c_device(
                        adapterBaseBusNum + device.channel, device.address
                    )
                    busNum = adapterBaseBusNum + device.channel
                    assert create_i2c_device(
                        device, busNum
                    ), f"i2c device {busNum}-00{device.address[2:]} not created"
                self.unload_kmods()

    def test_i2c_transactions(self) -> None:
        """
        Create bus, create devices on that bus, ensure that the bus
        driver can be unloaded successfully.
        """
        for fpga in self.fpgas:
            for adapter in fpga.i2cAdapters:
                # if any of the i2cDevices has testData
                if not any(device.testData for device in adapter.i2cDevices):
                    continue
                newAdapters, adapterBaseBusNum = create_i2c_adapter(fpga, adapter)
                for device in adapter.i2cDevices:
                    self.run_i2c_test_transactions(
                        device, adapterBaseBusNum + device.channel
                    )

    def run_i2c_test_transactions(self, device: I2CDevice, busNum: int) -> None:
        if not device.testData:
            return
        self.run_i2c_dump_test(device, busNum)
        self.run_i2c_get_test(device, busNum)

    def run_i2c_dump_test(self, device: I2CDevice, busNum: int) -> None:
        for tc in device.testData.i2cDumpData:
            output = run_cmd(
                [
                    "i2cdump",
                    "-y",
                    "-r",
                    f"{tc.start}-{tc.end}",
                    str(busNum),
                    device.address,
                ]
            ).stdout.decode()
            result = parse_i2cdump_data(output)
            assert (
                result == tc.expected
            ), f"i2cdump output {result} did not match expected: {tc.expected} at {tc.start}-{tc.end} on {device.address}"

    def run_i2c_get_test(self, device: I2CDevice, busNum: int) -> None:
        for tc in device.testData.i2cGetData:
            output = (
                run_cmd(["i2cget", "-y", str(busNum), device.address, tc.reg])
                .stdout.decode()
                .strip()
            )
            assert (
                output == tc.expected
            ), f"Output: {output} did not match expected: {tc.expected} at {tc.reg} on {device.address}"
