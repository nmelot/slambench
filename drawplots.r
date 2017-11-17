#!/usr/bin/env Rscript

library(ggplot2)
library(plyr)

options(error = function() {traceback(2); dump.frames; stopifnot(FALSE)})

drake = read.csv("drake-stats.csv", row.names = NULL, col.names = c("frame", "time", "x", "y", "z", "implementation", "width"))
cpp = read.csv("cpp-stats.csv", row.names = NULL, col.names = c("frame", "time", "x", "y", "z", "implementation", "width"))
openmp = read.csv("openmp-stats.csv", row.names = NULL, col.names = c("frame", "time", "x", "y", "z", "implementation", "width"))

data = rbind(drake, cpp)
data = rbind(data, openmp)

#drake_cpp = data.frame(frame = drake$frame, time = drake$time / cpp$time, implementation = "drake/cpp")
#drake_openmp = data.frame(frame = drake$frame, time = drake$time / openmp$time, implementation = "drake/openmp")
#openmp_cpp = data.frame(frame = drake$frame, time = openmp$time / cpp$time, implementation = "openmp/cpp")
#data = rbind(data, drake)
#data = rbind(data, cpp)
#data = rbind(data, openmp)

#data = ddply(data, c("implementation", "width"), summarize,
#	x = x - data[data$implementation == "cpp", "x"],
#	y = y - data[data$implementation == "cpp", "y"],
#	z = z - data[data$implementation == "cpp", "z"],
#	time = time,
#	frame = frame
#)

ref_width = 1
print("openmp / cpp")
mean(data[data$implementation == "openmp" & data$width == ref_width, "time"] / data[data$implementation == "cpp" & data$width == 1,"time"])
print("drake / cpp")
mean(data[data$implementation == "drake" & data$width == ref_width, "time"] / data[data$implementation == "cpp" & data$width == 1,"time"])
print("drake / openmp")
mean(data[data$implementation == "drake" & data$width == ref_width, "time"] / data[data$implementation == "openmp" & data$width == ref_width,"time"])

data = data[data$width == ref_width,]

plot = ggplot() +
  geom_line(data = data,
            aes(frame, time, group = implementation, color = implementation),
            size = 0.1) +
  #geom_point(data = apply_labels(frame[frame$freq == max(frame$freq) & frame$num_cores == max(frame$num_cores) & frame$size == max(frame$size) & frame$buffer_size == select_buffer_size,]),
  #          aes(tree_depth, global_mean_time, group = impl, color = impl, shape = impl),
  #          size = 5) +
  #geom_bar(data = apply_labels(frame[frame$freq == max(frame$freq) & frame$num_cores == max(frame$num_cores) & frame$size == max(frame$size) & frame$buffer_size == select_buffer_size,]),
  #  aes(tree_depth, global_mean_time, fill = impl),
  #  position = "dodge", stat = "identity") +
  guides(fill = guide_legend(title="Implementation"), colour = guide_legend(title = "Implementation"), point = guide_legend(title="Implementation"), shape = guide_legend(title = "Implementation")) +
  ylab("Processing time in seconds") +
  xlab("Frame") +
  ggtitle("Processing time for the living room 2 data set") +
  scale_fill_manual(values = colorRampPalette(c("#BB0000", "#66BB00", "#0000BB"))(6)) +
  scale_colour_manual(values = colorRampPalette(c("#BB0000", "#66BB00", "#0000BB"))(6))
#  scale_colour_manual(values = c("#771C19", "#B6C5CC"))
## Save the plot as a svg file
ggsave(file="time_per_implementation.svg", plot=plot, width=8, height=6)

plot = ggplot() +
  geom_line(data = data,
            aes(frame, x, group = implementation, color = implementation),
            size = 0.1) +
  #geom_point(data = apply_labels(frame[frame$freq == max(frame$freq) & frame$num_cores == max(frame$num_cores) & frame$size == max(frame$size) & frame$buffer_size == select_buffer_size,]),
  #          aes(tree_depth, global_mean_time, group = impl, color = impl, shape = impl),
  #          size = 5) +
  #geom_bar(data = apply_labels(frame[frame$freq == max(frame$freq) & frame$num_cores == max(frame$num_cores) & frame$size == max(frame$size) & frame$buffer_size == select_buffer_size,]),
  #  aes(tree_depth, global_mean_time, fill = impl),
  #  position = "dodge", stat = "identity") +
  guides(fill = guide_legend(title="Implementation"), colour = guide_legend(title = "Implementation"), point = guide_legend(title="Implementation"), shape = guide_legend(title = "Implementation")) +
  ylab("Processing time in seconds") +
  xlab("Frame") +
  ggtitle("Processing time for the living room 2 data set") +
  scale_fill_manual(values = colorRampPalette(c("#BB0000", "#66BB00", "#0000BB"))(6)) +
  scale_colour_manual(values = colorRampPalette(c("#BB0000", "#66BB00", "#0000BB"))(6))
#  scale_colour_manual(values = c("#771C19", "#B6C5CC"))
## Save the plot as a svg file
ggsave(file="xerror_per_implementation.svg", plot=plot, width=8, height=6)

plot = ggplot() +
  geom_line(data = data,
            aes(frame, y, group = implementation, color = implementation),
            size = 0.1) +
  #geom_point(data = apply_labels(frame[frame$freq == max(frame$freq) & frame$num_cores == max(frame$num_cores) & frame$size == max(frame$size) & frame$buffer_size == select_buffer_size,]),
  #          aes(tree_depth, global_mean_time, group = impl, color = impl, shape = impl),
  #          size = 5) +
  #geom_bar(data = apply_labels(frame[frame$freq == max(frame$freq) & frame$num_cores == max(frame$num_cores) & frame$size == max(frame$size) & frame$buffer_size == select_buffer_size,]),
  #  aes(tree_depth, global_mean_time, fill = impl),
  #  position = "dodge", stat = "identity") +
  guides(fill = guide_legend(title="Implementation"), colour = guide_legend(title = "Implementation"), point = guide_legend(title="Implementation"), shape = guide_legend(title = "Implementation")) +
  ylab("Processing time in seconds") +
  xlab("Frame") +
  ggtitle("Processing time for the living room 2 data set") +
  scale_fill_manual(values = colorRampPalette(c("#BB0000", "#66BB00", "#0000BB"))(6)) +
  scale_colour_manual(values = colorRampPalette(c("#BB0000", "#66BB00", "#0000BB"))(6))
#  scale_colour_manual(values = c("#771C19", "#B6C5CC"))
## Save the plot as a svg file
ggsave(file="yerror_per_implementation.svg", plot=plot, width=8, height=6)

plot = ggplot() +
  geom_line(data = data,
            aes(frame, z, group = implementation, color = implementation),
            size = 0.1) +
  #geom_point(data = apply_labels(frame[frame$freq == max(frame$freq) & frame$num_cores == max(frame$num_cores) & frame$size == max(frame$size) & frame$buffer_size == select_buffer_size,]),
  #          aes(tree_depth, global_mean_time, group = impl, color = impl, shape = impl),
  #          size = 5) +
  #geom_bar(data = apply_labels(frame[frame$freq == max(frame$freq) & frame$num_cores == max(frame$num_cores) & frame$size == max(frame$size) & frame$buffer_size == select_buffer_size,]),
  #  aes(tree_depth, global_mean_time, fill = impl),
  #  position = "dodge", stat = "identity") +
  guides(fill = guide_legend(title="Implementation"), colour = guide_legend(title = "Implementation"), point = guide_legend(title="Implementation"), shape = guide_legend(title = "Implementation")) +
  ylab("Processing time in seconds") +
  xlab("Frame") +
  ggtitle("Processing time for the living room 2 data set") +
  scale_fill_manual(values = colorRampPalette(c("#BB0000", "#66BB00", "#0000BB"))(6)) +
  scale_colour_manual(values = colorRampPalette(c("#BB0000", "#66BB00", "#0000BB"))(6))
#  scale_colour_manual(values = c("#771C19", "#B6C5CC"))
## Save the plot as a svg file
ggsave(file="zerror_per_implementation.svg", plot=plot, width=8, height=6)

