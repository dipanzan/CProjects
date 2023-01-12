from bcc import BPF
from ctypes import *

program = """
#include <uapi/linux/bpf.h>
#include <linux/futex.h>

#define TASK_COMM_LENGTH 16

struct data_t {
    u32 pid;
    u64 ts;
    char comm[TASK_COMM_LENGTH];
};

struct futex_data {
    uint32_t *uaddr1;
    int futex_op;
    uint32_t val;
    uint32_t val2;
    uint32_t *uaddr2;
    uint32_t val3;
};

BPF_PERF_OUTPUT(events);

int fn_process_hook(struct pt_regs *ctx)
{
    struct data_t data = {};
    data.pid = bpf_get_current_pid_tgid();
    data.ts = bpf_ktime_get_ns();
    bpf_get_current_comm(&data.comm, sizeof(data.comm));
    
    events.perf_submit(ctx, &data, sizeof(data));
    
    return 0;
}

int fn_futex_hook(struct pt_regs *ctx)
{   
    struct futex_data ftx_data = {};
    ftx_data.uaddr1 = (uint32_t *) PT_REGS_PARM1(ctx);
    ftx_data.futex_op = (int) PT_REGS_PARM2(ctx);
    ftx_data.val = (uint32_t) PT_REGS_PARM3(ctx);
    ftx_data.val2 = (uint32_t) PT_REGS_PARM4(ctx);
    ftx_data.uaddr2 = (uint32_t *) PT_REGS_PARM5(ctx);
    ftx_data.val3 = (uint32_t) PT_REGS_PARM6(ctx);
    
    //bpf_trace_printk("uaddr1: %x, futex_op: %d, val: %d", *(ftx_data.uaddr1), (ftx_data.futex_op), (ftx_data.val));
    //bpf_trace_printk("val2: %d", ftx_data.val2);
    
    events.perf_submit(ctx, &ftx_data, sizeof(ftx_data));
    
    return 0;
}
"""

b = BPF(text=program)
futex = b.get_syscall_fnname("futex")
b.attach_kprobe(event=futex, fn_name="fn_futex_hook")
#b.attach_kprobe(event=futex, fn_name="fn_process_hook")
#b.trace_print()

#print("%-18s %-16s %-6s %s" % ("TIME(s)", "COMM", "PID", "MESSAGE"))

# process event
# start = 0
# def print_event(cpu, data, size):
#     global start
#     event = b["events"].event(data)
#     if start == 0:
#         start = event.ts
#     time_s = (float(event.ts - start)) / 1000000000
#     print("%-18.9f %-16s %-6d %s" % (time_s, event.comm, event.pid, "futex() called"))

class Futex_Args(Structure):
    _fields_ = [
        ("uaddr1", c_ulong),
        ("futex_op", c_int),
        ("val", c_ulong),
        ("val2", c_ulong),
        ("uaddr2", c_ulong),
        ("val3", c_ulong)
    ]

def print_process_details(cpu, data, size):
    event = b["events"].event(data)
    print(event.comm)

def print_futex_event_args(cpu, data, size):
    futex_args = cast(data, POINTER(Futex_Args)).contents
    ftx_details = "uaddr1: {0}, futex_op: {1}, val: {2}, val2: {3}, uaddr2: {4}, val3: {5}".\
        format(futex_args.uaddr1, futex_args.futex_op, futex_args.val, futex_args.val2, futex_args.uaddr2, futex_args.val3)
    print(ftx_details)

def count_event():
    print("%10s %s" % ("COUNT", "STRING"))
    counts = b.get_table("counts")
    for k, v in sorted(counts.items(), key=lambda counts: counts[1].value):
        print("%10d \"%s\"" % (v.value, k.c.encode('string-escape')))

# loop with callback to print_event
b["events"].open_perf_buffer(print_futex_event_args)
#b["events"].open_perf_buffer(print_process_details)
while 1:
    try:
        b.perf_buffer_poll()
    except KeyboardInterrupt:
        exit()

