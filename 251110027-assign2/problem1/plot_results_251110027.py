import pandas as pd
import matplotlib.pyplot as plt


if __name__ == "__main__":
    # check if the results.csv file exists
    try:
        df = pd.read_csv("results.csv")
    except FileNotFoundError:
        print("ERROR: 'results.csv' not found. Run `make experiment` first.")
        exit(1)

    programs = ['sum', 'vec']
    scaling_types = ['Strong', 'Weak']
    locks = ['pthreads', 'abq', 'mcs']
    colors = {'pthreads': 'blue', 'abq': 'orange', 'mcs': 'green'}

    # Create a 2x2 grid of plots
    fig, axs = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Lock Scalability Evaluation (1 to 32 Threads)', fontsize=16)

    for row, scaling in enumerate(scaling_types):
        for col, prog in enumerate(programs):
            ax = axs[row, col]
            
            plot_data = df[(df['Scaling'] == scaling) & (df['Program'] == prog)]
            
            for lock in locks:
                lock_data = plot_data[plot_data['Lock'] == lock]
                ax.plot(lock_data['Threads'], lock_data['Time_ms'], 
                        marker='o', label=lock, color=colors[lock])
                
            ax.set_title(f'{scaling} Scaling - Shared {prog.capitalize()}')
            ax.set_xlabel('Number of Threads')
            ax.set_ylabel('Execution Time (ms)')
            ax.set_xticks(range(2, 33, 2))
            ax.grid(True, linestyle='--', alpha=0.7)
            ax.legend()

    plt.tight_layout()
    plt.subplots_adjust(top=0.92)
    plt.savefig('scalability_plots.png', dpi=300)
    print("Plots saved successfully as 'scalability_plots.png'")