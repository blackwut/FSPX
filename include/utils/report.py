# read vitis_hls.log and extract the performance data
# things to extract:
# INFO: [HLS 200-42] -- Implementing module
# INFO: [HLS 200-1470] Pipelining result : Target II =
# INFO: [HLS 200-789] **** Estimated Fmax:

import sys
import argparse
import shlex

def load_file(file):
    with open(file, 'r') as f:
        return f.readlines()


def ii_string_to_int(ii_string):
    if ii_string == 'NA':
        return -1
    return int(ii_string)


def extract_performance_data(log):
    data = []

    module = ''
    loop_name = ''
    target_ii = ''
    final_ii = ''
    depth = ''
    fmax = ''

    is_warning = False

    for i, line in enumerate(log):
        if '[HLS 200-42]' in line:
        # "INFO: [HLS 200-42] -- Implementing module 'module' \n"
            module = line.split()[-1].replace('\'', '')

        if '[HLS 200-1470]' in line:
        # "INFO: [HLS 200-1470] Pipelining result : Target II = 1, Final II = 1, Depth = 1, loop 'LOOP_NAME' \n"
            line_split = shlex.split(line)
            target_ii = ii_string_to_int(line_split[-10].replace(',', ''))
            final_ii = ii_string_to_int(line_split[-6].replace(',', ''))
            depth = line_split[-3].replace(',', '')
            loop_name = line_split[-1].replace('\'', '')

            data.append({
                'Module': module,
                'LoopName': loop_name,
                'Target II': target_ii,
                'Final II': final_ii,
                'Depth': depth,
            })

        # WARNING: [HLS 200-871] Estimated clock period (3.684 ns) exceeds the target (target clock period: 3.330 ns, clock uncertainty: 0.899 ns, effective delay budget: 2.431 ns).
        if '[HLS 200-871]' in line:
            data.append({
                'WARNING': line,
                'Final II': 0
            })
            is_warning = True
            continue
        if is_warning:
            if line == '\n':
                is_warning = False
            else:
                data.append({
                    'WARNING': line,
                    'Final II': 0
                })

        if '[HLS 200-789]' in line:
        # "INFO: [HLS 200-789] **** Estimated Fmax: 300.00 MHz \n"
            fmax = line.split()[-2]

    return data, fmax


def extract_warnings(log):
    warnings = []
    for i, line in enumerate(log):
        if 'WARNING' in line:
            # remove if the line contains "Legalizing"
            if 'Legalizing' in line:
                continue
            warnings.append(line)
    return warnings

# main
if __name__ == '__main__':
    log_file = 'vitis_hls.log'
    
    parser = argparse.ArgumentParser()
    parser.add_argument('log_file', type=str, help='Path to the log file', default='vitis_hls.log', nargs='?')
    parser.add_argument('--warnings', action='store_true', help='Print warnings')
    parser.add_argument('--performance', action='store_true', help='Print performance data')

    args = parser.parse_args()

    log_file = args.log_file
    print_warnings = args.warnings
    print_performance = args.performance


    log = load_file(log_file)

    if print_warnings:
        warnings = extract_warnings(log)
        for warning in warnings:
            print(warning, end='')

    if print_performance:
        data, fmax = extract_performance_data(log)

        print(f'\033[92mFMax: {fmax} MHz --- II = {max([row["Final II"] for row in data])}\033[0m')
        for row in data:
            if 'WARNING' in row:
                print('\033[91m' + row['WARNING'] + '\033[0m', end='')
                continue

            print()
            for key, value in row.items():
                print(f'{key.rjust(10)}: {str(value)}')