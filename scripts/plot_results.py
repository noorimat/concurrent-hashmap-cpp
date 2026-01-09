import matplotlib.pyplot as plt
import numpy as np

# Data from your benchmark results
threads = [1, 2, 4, 8]

# Read-Only workload
read_lockfree = [0.36, 0.36, 0.43, 0.70]
read_mutex = [0.49, 1.55, 5.89, 16.08]

# Insert-Only workload
insert_lockfree = [0.80, 0.98, 1.29, 3.08]
insert_mutex = [2.40, 8.64, 21.92, 68.28]

# Mixed 50/50 workload
mixed_lockfree = [2.67, 4.42, 18.70, 32.67]
mixed_mutex = [1.84, 7.30, 25.19, 70.46]

# Read-Heavy 80/20 workload
readheavy_lockfree = [1.79, 3.21, 6.27, 15.46]
readheavy_mutex = [1.15, 4.46, 12.23, 37.71]

# Create figure with 2x2 subplots
fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(14, 10))

# Read-Only
ax1.plot(threads, read_lockfree, 'o-', label='Lock-Free', linewidth=2, markersize=8)
ax1.plot(threads, read_mutex, 's-', label='Mutex-Based', linewidth=2, markersize=8)
ax1.set_xlabel('Thread Count', fontsize=12)
ax1.set_ylabel('Time (ms)', fontsize=12)
ax1.set_title('Read-Only Workload', fontsize=14, fontweight='bold')
ax1.legend(fontsize=10)
ax1.grid(True, alpha=0.3)
ax1.set_xticks(threads)

# Insert-Only
ax2.plot(threads, insert_lockfree, 'o-', label='Lock-Free', linewidth=2, markersize=8)
ax2.plot(threads, insert_mutex, 's-', label='Mutex-Based', linewidth=2, markersize=8)
ax2.set_xlabel('Thread Count', fontsize=12)
ax2.set_ylabel('Time (ms)', fontsize=12)
ax2.set_title('Insert-Only Workload', fontsize=14, fontweight='bold')
ax2.legend(fontsize=10)
ax2.grid(True, alpha=0.3)
ax2.set_xticks(threads)

# Mixed 50/50
ax3.plot(threads, mixed_lockfree, 'o-', label='Lock-Free', linewidth=2, markersize=8)
ax3.plot(threads, mixed_mutex, 's-', label='Mutex-Based', linewidth=2, markersize=8)
ax3.set_xlabel('Thread Count', fontsize=12)
ax3.set_ylabel('Time (ms)', fontsize=12)
ax3.set_title('Mixed 50/50 Workload', fontsize=14, fontweight='bold')
ax3.legend(fontsize=10)
ax3.grid(True, alpha=0.3)
ax3.set_xticks(threads)

# Read-Heavy 80/20
ax4.plot(threads, readheavy_lockfree, 'o-', label='Lock-Free', linewidth=2, markersize=8)
ax4.plot(threads, readheavy_mutex, 's-', label='Mutex-Based', linewidth=2, markersize=8)
ax4.set_xlabel('Thread Count', fontsize=12)
ax4.set_ylabel('Time (ms)', fontsize=12)
ax4.set_title('Read-Heavy 80/20 Workload', fontsize=14, fontweight='bold')
ax4.legend(fontsize=10)
ax4.grid(True, alpha=0.3)
ax4.set_xticks(threads)

plt.suptitle('Lock-Free HashMap Performance Scaling', fontsize=16, fontweight='bold')
plt.tight_layout()
plt.savefig('performance_scaling.png', dpi=300, bbox_inches='tight')
print("✓ Saved performance_scaling.png")

# Create speedup chart
fig2, ax = plt.subplots(figsize=(10, 6))

read_speedup = [read_mutex[i] / read_lockfree[i] for i in range(4)]
insert_speedup = [insert_mutex[i] / insert_lockfree[i] for i in range(4)]
mixed_speedup = [mixed_mutex[i] / mixed_lockfree[i] for i in range(4)]
readheavy_speedup = [readheavy_mutex[i] / readheavy_lockfree[i] for i in range(4)]

x = np.arange(len(threads))
width = 0.2

ax.bar(x - 1.5*width, read_speedup, width, label='Read-Only', color='#2ecc71')
ax.bar(x - 0.5*width, insert_speedup, width, label='Insert-Only', color='#3498db')
ax.bar(x + 0.5*width, mixed_speedup, width, label='Mixed 50/50', color='#e74c3c')
ax.bar(x + 1.5*width, readheavy_speedup, width, label='Read-Heavy 80/20', color='#f39c12')

ax.axhline(y=1, color='black', linestyle='--', linewidth=1, alpha=0.5)
ax.set_xlabel('Thread Count', fontsize=12)
ax.set_ylabel('Speedup vs Mutex-Based', fontsize=12)
ax.set_title('Lock-Free HashMap Speedup Comparison', fontsize=14, fontweight='bold')
ax.set_xticks(x)
ax.set_xticklabels(threads)
ax.legend(fontsize=10)
ax.grid(True, alpha=0.3, axis='y')

plt.tight_layout()
plt.savefig('speedup_comparison.png', dpi=300, bbox_inches='tight')
print("✓ Saved speedup_comparison.png")

print("\n📊 Generated 2 performance graphs!")