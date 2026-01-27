# Ares

TizenRT application which includes various functional tests.

## Prerequisites
1. AILITE BOARD
2. Docker
3. GIT

## Usage

1. Clone the [TizenRT](https://github.com/Samsung/TizenRT) repository
2. `cd` into `apps/examples` of the cloned TizenRT repository
3. Clone this repository into the CWD
4. Run `git submodule update --init --recursive`

## Building the application

1. `cd` into `os/` directory of the TizenRT repository
2. Run `./dbuild.sh menu`
3. Ensure configuration is set as:
  1. RTL8730E
  2. loadable_ext_xxx (Depending on board type)
4. Run `"3. Modify Current Configuration"`
5. In `Application Configuration > Examples`, enable `Ares` example.
6. Run `1. Build with Current Configuration`

## Formatting

Please apply the `.clang-format` before committing changes.
You may use `CTRL+SHIFT+P > Format Document` in VSCode

## Task analysis

Estimates only all in ms

| Task Name           | Period (T)       | Execution Time (C)                     | Deadline (D) | Critial? | Utilization |
| :------------------ | :--------------- | :------------------------------------- | :----------- | :------- | :---------- |
| lvgl_tick           | 5                | ~0.1                                   | 5            | Yes      | 0.02        |
| lvgl_drawer         | 5                | C_timer + (C_text / 200) = 0.82        | 5            | Yes      | 0.164       |
| lcd_power           | 5000 * 3 = 15000 | C_poweron + C_poweroff = 2             | 15000        | No       | 0           |
| wifi                | 3000             | C_wifi + C_mq + C_malloc = 300         | 3000         | No       | 0.1         |
| fs_mnt_reader       | 5000             | C_read + C_verify = 300 + 100          | 5000         | No       | 0.08        |
| fs_mnt_writer       | 5000             | C_write = 1000                         | 5000         | No       | 0.2         |
| fs_mnt0_reader      | 5000             | C_read + C_verify = 300 + 100          | 5000         | No       | 0.08        |
| fs_mnt0_writer      | 5000             | C_write = 1000                         | 5000         | No       | 0.2         |
| uart_rx             | 3000             | C_rx = 200                             | 3000         | No       | 0.067       |
| power               | N.A.             | N.A.                                   | N.A.         | No       | 0           |
| monitor             | 3000             | C_heap = 100                           | 3000         | No       | 0.033       |
| audio (+play_music) | 5000             | C_pthread_create + C_audio = 200 + 500 | 5000         | Yes      | 0.14        |

Utilization (excluding default tasks)

| CPU | Utilization |
| :-- | :---------- |
| 0   | 0.9         |
| 1   | 0.184       |


