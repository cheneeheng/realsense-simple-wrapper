import argparse
import time
import tqdm

from rs_py import printout
from rs_py import get_rs_parser
from rs_py import RealsenseWrapper


def test_device_init(args: argparse.Namespace):
    """Runs test on device initialization.

    Args:
        args (Namespace): args from cli.
    """
    args.save_data = False
    rsw = RealsenseWrapper(args, args.rs_dev)
    rsw.initialize()
    rsw.set_ir_laser_power(args.rs_laser_power)
    rsw.dummy_capture(args.rs_fps * 5)
    rsw.stop()

    N = 100
    t = time.time()
    for _ in tqdm.tqdm(range(N)):
        rsw.initialize()
        rsw.stop()

    printout(f"Finished in {(time.time()-t)/N}", 'i')


def run_devices(args: argparse.Namespace):
    """Runs realsense devices.

    Args:
        args (Namespace): args from cli.
    """

    rsw = RealsenseWrapper(args, args.rs_dev)
    rsw.initialize()
    rsw.set_ir_laser_power(args.rs_laser_power)
    rsw.save_calibration()

    rsw.dummy_capture(args.rs_fps * 5)

    try:
        c = 0
        max_c = int(1e8)

        while True:

            if c % args.rs_fps == 0:
                printout(f"Step {c:8d}", 'i')

            frames = rsw.step(
                display=args.rs_display_frame,
                display_and_save_with_key=args.rs_save_with_key
            )

            if not len(frames) > 0:
                printout(f"Empty...", 'w')
                continue

            c += 1
            if c > args.rs_fps * args.rs_steps or c > max_c:
                break

    except Exception as e:
        printout(f"{e}", 'e')
        printout(f"Stopping RealSense devices...", 'i')
        rsw.stop()

    finally:
        printout(f"Final RealSense devices...", 'i')
        rsw.stop()

    printout(f"Finished...", 'i')


if __name__ == "__main__":
    args = get_rs_parser().parse_args()
    print("========================================")
    print(">>>>> args <<<<<")
    print("========================================")
    for k, v in vars(args).items():
        print(f"{k} : {v}")
    print("========================================")

    if args.rs_test_init_runtime:
        test_device_init(args)
    else:
        run_devices(args)
