import sys
import subprocess
from pathlib import Path
from multiprocessing import Pool

# path, url, commit (Alphabet sorted by path)
deps_list = {
    'hw/mcu/allwinner'                            : [ 'https://github.com/hathach/allwinner_driver.git',                '8e5e89e8e132c0fd90e72d5422e5d3d68232b756'],
    'hw/mcu/bridgetek/ft9xx/ft90x-sdk'            : [ 'https://github.com/BRTSG-FOSS/ft90x-sdk.git',                    '91060164afe239fcb394122e8bf9eb24d3194eb1'],
    'hw/mcu/broadcom'                             : [ 'https://github.com/adafruit/broadcom-peripherals.git',           '08370086080759ed54ac1136d62d2ad24c6fa267'],
    'hw/mcu/gd/nuclei-sdk'                        : [ 'https://github.com/Nuclei-Software/nuclei-sdk.git',              '7eb7bfa9ea4fbeacfafe1d5f77d5a0e6ed3922e7'],
    'hw/mcu/infineon/mtb-xmclib-cat3'             : [ 'https://github.com/Infineon/mtb-xmclib-cat3.git',                'daf5500d03cba23e68c2f241c30af79cd9d63880'],
    'hw/mcu/microchip'                            : [ 'https://github.com/hathach/microchip_driver.git',                '9e8b37e307d8404033bb881623a113931e1edf27'],
    'hw/mcu/mindmotion/mm32sdk'                   : [ 'https://github.com/hathach/mm32sdk.git',                         '708a7152952ac595d24837069dcc0f7f59a4c30b'],
    'hw/mcu/nordic/nrfx'                          : [ 'https://github.com/NordicSemiconductor/nrfx.git',                '281cc2e178fd9a470d844b3afdea9eb322a0b0e8'],
    'hw/mcu/nuvoton'                              : [ 'https://github.com/majbthrd/nuc_driver.git',                     '2204191ec76283371419fbcec207da02e1bc22fa'],
    'hw/mcu/nxp/lpcopen'                          : [ 'https://github.com/hathach/nxp_lpcopen.git',                     '43c45c85405a5dd114fff0ea95cca62837740c13'],
    'hw/mcu/nxp/mcux-sdk'                         : [ 'https://github.com/NXPmicro/mcux-sdk.git',                       'ae2ab01d9d70ad00cd0e935c2552bd5f0e5c0294'],
    'hw/mcu/nxp/nxp_sdk'                          : [ 'https://github.com/hathach/nxp_sdk.git',                         '845c8fc49b6fb660f06a5c45225494eacb06f00c'],
    'hw/mcu/raspberry_pi/Pico-PIO-USB'            : [ 'https://github.com/sekigon-gonnoc/Pico-PIO-USB.git',             '9ff3f52fd3c1f81532bce8dd311aa8fc8d9b2665'],
    'hw/mcu/renesas/rx'                           : [ 'https://github.com/kkitayam/rx_device.git',                      '706b4e0cf485605c32351e2f90f5698267996023'],
    'hw/mcu/silabs/cmsis-dfp-efm32gg12b'          : [ 'https://github.com/cmsis-packs/cmsis-dfp-efm32gg12b.git',        'f1c31b7887669cb230b3ea63f9b56769078960bc'],
    'hw/mcu/sony/cxd56/spresense-exported-sdk'    : [ 'https://github.com/sonydevworld/spresense-exported-sdk.git',     '2ec2a1538362696118dc3fdf56f33dacaf8f4067'],
    'hw/mcu/st/cmsis_device_f0'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_f0.git',      '2fc25ee22264bc27034358be0bd400b893ef837e'],
    'hw/mcu/st/cmsis_device_f1'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_f1.git',      '6601104a6397299b7304fd5bcd9a491f56cb23a6'],
    'hw/mcu/st/cmsis_device_f2'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_f2.git',      '182fcb3681ce116816feb41b7764f1b019ce796f'],
    'hw/mcu/st/cmsis_device_f3'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_f3.git',      '5e4ee5ed7a7b6c85176bb70a9fd3c72d6eb99f1b'],
    'hw/mcu/st/cmsis_device_f4'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_f4.git',      '2615e866fa48fe1ff1af9e31c348813f2b19e7ec'],
    'hw/mcu/st/cmsis_device_f7'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_f7.git',      'fc676ef1ad177eb874eaa06444d3d75395fc51f4'],
    'hw/mcu/st/cmsis_device_g0'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_g0.git',      '08258b28ee95f50cb9624d152a1cbf084be1f9a5'],
    'hw/mcu/st/cmsis_device_g4'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_g4.git',      'ce822adb1dc552b3aedd13621edbc7fdae124878'],
    'hw/mcu/st/cmsis_device_h7'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_h7.git',      '60dc2c913203dc8629dc233d4384dcc41c91e77f'],
    'hw/mcu/st/cmsis_device_l0'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_l0.git',      '06748ca1f93827befdb8b794402320d94d02004f'],
    'hw/mcu/st/cmsis_device_l1'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_l1.git',      '7f16ec0a1c4c063f84160b4cc6bf88ad554a823e'],
    'hw/mcu/st/cmsis_device_l4'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_l4.git',      '6ca7312fa6a5a460b5a5a63d66da527fdd8359a6'],
    'hw/mcu/st/cmsis_device_l5'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_l5.git',      'd922865fc0326a102c26211c44b8e42f52c1e53d'],
    'hw/mcu/st/cmsis_device_u5'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_u5.git',      'bc00f3c9d8a4e25220f84c26d414902cc6bdf566'],
    'hw/mcu/st/cmsis_device_wb'                   : [ 'https://github.com/STMicroelectronics/cmsis_device_wb.git',      '9c5d1920dd9fabbe2548e10561d63db829bb744f'],
    'hw/mcu/st/stm32f0xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32f0xx_hal_driver.git', '0e95cd88657030f640a11e690a8a5186c7712ea5'],
    'hw/mcu/st/stm32f1xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32f1xx_hal_driver.git', '1dd9d3662fb7eb2a7f7d3bc0a4c1dc7537915a29'],
    'hw/mcu/st/stm32f2xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32f2xx_hal_driver.git', 'c75ace9b908a9aca631193ebf2466963b8ea33d0'],
    'hw/mcu/st/stm32f3xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32f3xx_hal_driver.git', '1761b6207318ede021706e75aae78f452d72b6fa'],
    'hw/mcu/st/stm32f4xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32f4xx_hal_driver.git', '04e99fbdabd00ab8f370f377c66b0a4570365b58'],
    'hw/mcu/st/stm32f7xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32f7xx_hal_driver.git', 'f7ffdf6bf72110e58b42c632b0a051df5997e4ee'],
    'hw/mcu/st/stm32g0xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32g0xx_hal_driver.git', '5b53e6cee664a82b16c86491aa0060e2110c00cb'],
    'hw/mcu/st/stm32g4xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32g4xx_hal_driver.git', '8b4518417706d42eef5c14e56a650005abf478a8'],
    'hw/mcu/st/stm32h7xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32h7xx_hal_driver.git', 'd8461b980b59b1625207d8c4f2ce0a9c2a7a3b04'],
    'hw/mcu/st/stm32l0xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32l0xx_hal_driver.git', 'fbdacaf6f8c82a4e1eb9bd74ba650b491e97e17b'],
    'hw/mcu/st/stm32l1xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32l1xx_hal_driver.git', '44efc446fa69ed8344e7fd966e68ed11043b35d9'],
    'hw/mcu/st/stm32l4xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32l4xx_hal_driver.git', 'aee3d5bf283ae5df87532b781bdd01b7caf256fc'],
    'hw/mcu/st/stm32l5xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32l5xx_hal_driver.git', '675c32a75df37f39d50d61f51cb0dcf53f07e1cb'],
    'hw/mcu/st/stm32u5xx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32u5xx_hal_driver.git', '2e1d4cdb386e33391cb261dfff4fefa92e4aa35a'],
    'hw/mcu/st/stm32wbxx_hal_driver'              : [ 'https://github.com/STMicroelectronics/stm32wbxx_hal_driver.git', '2c5f06638be516c1b772f768456ba637f077bac8'],
    'hw/mcu/ti'                                   : [ 'https://github.com/hathach/ti_driver.git',                       '143ed6cc20a7615d042b03b21e070197d473e6e5'],
    'hw/mcu/wch/ch32v307'                         : [ 'https://github.com/openwch/ch32v307.git',                        '17761f5cf9dbbf2dcf665b7c04934188add20082'],
    'lib/CMSIS_5'                                 : [ 'https://github.com/ARM-software/CMSIS_5.git',                    '20285262657d1b482d132d20d755c8c330d55c1f'],
    #'lib/FreeRTOS-Kernel'                         : [ 'https://github.com/FreeRTOS/FreeRTOS-Kernel.git',                '2a604f4a2818b8354b5e1a39e388eb5e16cfbc1f'],
    #'lib/lwip'                                    : [ 'https://github.com/lwip-tcpip/lwip.git',                         '159e31b689577dbf69cf0683bbaffbd71fa5ee10'],
    'lib/sct_neopixel'                            : [ 'https://github.com/gsteiert/sct_neopixel.git',                   'e73e04ca63495672d955f9268e003cffe168fcd8'],
    #'tools/uf2'                                   : [ 'https://github.com/microsoft/uf2.git',                           '19615407727073e36d81bf239c52108ba92e7660'],
}

# TOP is tinyusb root dir
TOP = Path(__file__).parent.parent.resolve()


def get_a_dep(d):
    if d not in deps_list.keys():
        print('{} is not found in dependency list')
        return 1
    url = deps_list[d][0]
    commit = deps_list[d][1]
    print('cloning {} with {}'.format(d, url))

    p = Path(TOP / d)
    git_cmd = "git -C {}".format(p)

    # Init git deps if not existed
    if not p.exists():
        p.mkdir(parents=True)
        subprocess.run("{} init".format(git_cmd), shell=True)
        subprocess.run("{} remote add origin {}".format(git_cmd, url), shell=True)

    # Check if commit is already fetched
    result = subprocess.run("{} rev-parse HEAD".format(git_cmd, commit), shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    head = result.stdout.decode("utf-8").splitlines()[0]

    if commit != head:
        subprocess.run("{} fetch --depth 1 origin {}".format(git_cmd, commit), shell=True)
        subprocess.run("{} checkout FETCH_HEAD".format(git_cmd), shell=True)

    return 0


if __name__ == "__main__":
    status = 0
    all_deps = sys.argv[1:]
    with Pool() as pool:
        result = pool.map(get_a_dep, all_deps)
        status = sum(result)
    sys.exit(status)
