#!/usr/bin/python
import matplotlib
matplotlib.use('agg')
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.pyplot import cm
import matplotlib.patches as patches
import sys

MAX_ORDER_NUM = 10

color = iter(cm.hsv(np.linspace(0, 1, MAX_ORDER_NUM)))

order_colors = []
for i in range(MAX_ORDER_NUM):
    order_colors.append(next(color))

# print order_colors
events = {}
machineLineMap = {}

def main(args):
    if (len(args) < 1):
        raise Exception("Missing Input File")

    # parse the input file now
    f = open(args[0], "r")
    lines = f.readlines()
    f.close()
    
    line_index = 0
    # Setup Plot
    fig, ax = plt.subplots(figsize=(30, 15))
    makespan = 0
    mach_labs_pos = []
    mach_labs = []
    order_counter = 0
    machine_counter = 0

    #Attributes
    mc_id = -1
    line_id = -1
    line_name = ''
    order_id = -1
    prod_name = ''
    prod_id = -1
    order_q = -1
    order_st = -1
    order_et = -1
    y = -5

    while(line_index < len(lines)):
        l = lines[line_index].strip()
        sp = l.split()
        line_index += 1

        if (l.startswith("Machine")):
            mc_id = int(sp[2])
            line_id = int(sp[5])
            line_name = sp[8]
            print("Machine", mc_id, line_id, line_name)

            if ('AA' in line_name):
                continue
            elif 'AD' in line_name:
                line_sub_id = line_name[-1]
                line_name += ' / AA' + line_sub_id 

            #Set Plotting Attributes
            y += 5
            mach_labs_pos.append(y + 2)
            mach_labs.append("Line " + line_name)
            machine_counter += 1
            continue

        if (l.startswith("Order")):
            order_id = int(sp[2])
            prod_name = sp[6]
            prod_id = int(sp[10])
            order_q = int(sp[13])
            order_st = int(sp[17])
            order_et = int(sp[21])
            print("Order", order_id, prod_name, prod_id, order_q, order_st, order_et)

            #Plot Order
            op_start = order_st / 60  # Convert to mins, start value is in decisecs
            op_end = order_et  / 60 # Convert to mins
            print("Converted Time", op_start, op_end)
            makespan = max(makespan, order_et)
            ax.add_patch(patches.Rectangle(
                (op_start, y),   # (x,y)
                op_end - op_start,          # width
                5.0,          # height
                fill=True,
                alpha=0.6,
                linestyle='--',
                edgecolor="black",
                facecolor=order_colors[order_counter % 10]
                # hatch='+'
            ))

            new_fontsize = ((op_end - op_start) / 100.0) * 5

            if (op_end - op_start > 5):
                tex_cx = op_start + (op_end - op_start) / 2.0
                tex_cy = y + 2.5
                tex = "Order#" + str(order_id)
                ax.annotate(tex, (tex_cx, tex_cy), color='b', weight='bold',
                            fontsize=new_fontsize, ha='center', va='center')
            
            #Increase counters
            order_counter += 1
            continue

        '''
        l = f.readline().rstrip().split(" ")
        print l
        mach_ID, mach_Op_Num, mach_line_id, mach_line_name = l
        mach_ID = int(mach_ID)
        mach_Op_Num = int(mach_Op_Num)
        if not mach_Op_Num:
            continue
        y = counter * 10
        mach_labs_pos.append(y + 5)
        mach_labs.append("Line " + mach_line_name + " Machine" + str(i))

        for j in range(mach_Op_Num):
            op_uid, order_id, op_job, op_id, op_start, op_end = map(
                int, f.readline().split(" "))
            op_start = op_start / 600.0  # Convert to mins, start value is in decisecs
            op_end = op_end / 600.0  # Convert to mins
            makespan = max(makespan, op_end)
            ax.add_patch(patches.Rectangle(
                (op_start, y),   # (x,y)
                op_end - op_start,          # width
                10.0,          # height
                fill=True,
                alpha=0.6,
                linestyle='--',
                edgecolor="black",
                facecolor=order_colors[order_id]
                # hatch='+'
            ))
            tex_cx = op_start + (op_end - op_start) / 2.0
            tex_cy = y + 5.0
            #tex = "Job: " + str(op_job) + " ID: " + str(op_id)
            tex = "Order#" + str(order_id) + "_#" + str(op_job)
            ax.annotate(tex, (tex_cx, tex_cy), color='b', weight='bold',
                        fontsize=10, ha='center', va='center')
        counter += 1
        '''


    '''
    #Add Patches for events
    ax.set_yticks(mach_labs_pos)
    ax.set_yticklabels(mach_labs)
    ax.set_xlim([0, makespan])
    ax.set_ylim([0, counter * 10])
    ax.set_title('GANTT CHART')

    if (args[1] != '0'):
        print "Plotting Rescheduling Line", float(args[1])
        hor_x = float(args[1]) / 600.0
        ax.axvline(x=hor_x, color='k', linestyle='--')
        ax.add_patch(patches.Rectangle(
                (0, 0),   # (x,y)
                hor_x,          # width
                counter*10,          # height
                fill=True,
                alpha=0.3,
                edgecolor="black",
                facecolor='r'
                # hatch='+'
            ))
    
    fig.savefig('gant.png', dpi=300, bbox_inches='tight')
    # plt.show()
    '''
    ax.set_yticks(mach_labs_pos)
    ax.set_yticklabels(mach_labs)
    ax.set_xlim([0, 10 + makespan / 60])
    ax.set_ylim([0, machine_counter * 5])
    ax.set_title('GANTT CHART')

    print("Saving Plot")
    fig.savefig('gant.png', dpi=300, bbox_inches='tight')
    #plt.show()



if (__name__ == "__main__"):
    try:
        main([r"test_output.out", sys.argv[1]])
    except:
        print ("Missing CMD argument")
