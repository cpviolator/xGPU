# Allow local customizations to build process
# (usually sizing for a specific project)
-include Makefile.local

CUDA_DIR ?= /usr/local/cuda-12.2
ifneq ($(strip $(OSTYPE)),osx)
CUDA_LIBDIR ?= $(CUDA_DIR)/lib64
else
CUDA_LIBDIR ?= $(CUDA_DIR)/lib
endif

DEBUG ?= -O3
TEXTURE_DIM ?= 1
CUDA_ARCH ?= sm_70

NVCC = $(CUDA_DIR)/bin/nvcc
prefix ?= /usr/local
bindir ?= $(prefix)/bin
libdir ?= $(prefix)/lib
includedir ?= $(prefix)/include

ifeq ($(strip $(OSTYPE)),cygwin)
NVCCFLAGS = --compiler-bindir "c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin" -Xcompiler /openmp
LFLAGS = -L"c:\Program Files\Microsoft SDKs\Windows\v7.1\Lib" -L.
RPATH =
LIBXGPU = libxgpu.dll
else

NVCCFLAGS = -Xcompiler -D_REENTRANT -Xcompiler -Wall -Xcompiler -Wno-unused-function -ftz=true

ifneq ($(strip $(OSTYPE)),osx)
NVCCFLAGS += -Xcompiler -fopenmp 
else
NVCCFLAGS += -Xcompiler -Wno-unknown-pragmas -Xcompiler -Wno-gnu-designator
endif

ifneq ($(strip $(OSTYPE)),osx)
LFLAGS = -L$(CUDA_LIBDIR) -L. -lrt
else
LFLAGS = -L$(CUDA_LIBDIR) -L. -lcudart
endif

RPATH = -Xlinker -rpath,'$${ORIGIN}:$${ORIGIN}/../lib'
LIBXGPU = libxgpu.so
endif

#OLD
#NVCCFLAGS += $(DEBUG) -arch=$(CUDA_ARCH) -Xptxas -abi=no --ptxas-options=-v -prec-sqrt=false -Xcompiler -fPIC
#NVCCFLAGS += -DTEXTURE_DIM=$(TEXTURE_DIM)

#NEW
NVCCFLAGS += $(DEBUG) -arch=$(CUDA_ARCH) 
#NVCCFLAGS += -DTEXTURE_DIM=$(TEXTURE_DIM)

ifeq ($(strip $(CUDA_ARCH)),sm_35)
NVCCFLAGS += -maxrregcount=72
endif

ifeq ($(strip $(CUDA_ARCH)),sm_50)
NVCCFLAGS += -maxrregcount=72
endif

ifeq ($(strip $(CUDA_ARCH)),sm_52)
NVCCFLAGS += -maxrregcount=72
endif

#NVCCFLAGS += -Xptxas -dlcm=cg # disable L1 cache

# Possibly (re-)build xgpu_version.h
$(shell $(SHELL) gen_xgpu_version.sh)

##########################
# Sizing variable handlers
##########################

ifdef NPOL
NVCCFLAGS += -DNPOL=$(NPOL)
endif

ifdef NSTATION
NVCCFLAGS += -DNSTATION=$(NSTATION)
else
# Handle common typo
ifdef NSTATIONS
NVCCFLAGS += -DNSTATION=$(NSTATIONS)
endif
endif

ifdef NFREQUENCY
NVCCFLAGS += -DNFREQUENCY=$(NFREQUENCY)
endif

ifdef NTIME
NVCCFLAGS += -DNTIME=$(NTIME)
endif

ifdef NTIME_PIPE
NVCCFLAGS += -DNTIME_PIPE=$(NTIME_PIPE)
else
# Handle common typo
ifdef NTIMEPIPE
NVCCFLAGS += -DNTIME_PIPE=$(NTIMEPIPE)
endif
endif

ifdef SHARED_ATOMIC_SIZE
NVCCFLAGS += -DSHARED_ATOMIC_SIZE=$(SHARED_ATOMIC_SIZE)
endif

ifdef COMPLEX_BLOCK_SIZE
NVCCFLAGS += -DCOMPLEX_BLOCK_SIZE=$(COMPLEX_BLOCK_SIZE)
endif

################################
# Benchmarking variable handlers
################################

# this sets benchmarking mode - no correctness testing done
ifeq ($(strip $(BENCHMARK)), yes)
NVCCFLAGS += -DBENCHMARK
endif

# this sets the pipeline to run in an infinite loop - for power measurement
ifeq ($(strip $(POWER_LOOP)), yes)
NVCCFLAGS += -DPOWER_LOOP
endif

ifdef RUNTIME_STATS
NVCCFLAGS += -DRUNTIME_STATS
endif

# Handle V=1 for verbose output
ifneq "$V" "1"
	VERBOSE=@
endif

CUDA_CORRELATOR_OBJS  = cuda_correlator.o

XGPUINFO_OBJS  = xgpuinfo.o

# Objects for executables
BINXGPU_OBJS = $(CUDA_CORRELATOR_OBJS) $(XGPUINFO_OBJS)

# Objects for library
LIBXGPU_OBJS  = cuda_xengine.o
LIBXGPU_OBJS += omp_xengine.o
LIBXGPU_OBJS += cpu_util.o

# Each object file has a corresponding dependency file
DEPS = $(BINXGPU_OBJS:.o=.d) $(LIBXGPU_OBJS:.o=.d)

all: cuda_correlator xgpuinfo $(LIBXGPU)

cuda_correlator: $(CUDA_CORRELATOR_OBJS) $(LIBXGPU)
	$(NVCC) $(NVCCFLAGS) $(CUDA_CORRELATOR_OBJS) -o $@ $(LFLAGS) $(RPATH) -lxgpu -lnvidia-ml

xgpuinfo: $(XGPUINFO_OBJS) $(LIBXGPU)
	$(NVCC) $(NVCCFLAGS) $(XGPUINFO_OBJS) -o $@ $(LFLAGS) $(RPATH) -lxgpu -lnvidia-ml

$(LIBXGPU): $(LIBXGPU_OBJS)
	$(NVCC) $(NVCCFLAGS) $^ -o $@ $(LFLAGS) --shared

# A target to make all object files, but not link them
objs: $(BINXGPU_OBJS) $(LIBXGPU_OBJS)

# Include dependencies after explicit rules (really anywhere after first
# explicit rule) so that they do not provide default target.
-include $(DEPS)

# These .{c,cc,cu} -> .o rules will generate dependency files as a side effect
# of the compilation.  These files are included elsewhere in this Makefile.
#
# NB Include dep files using "-include", see
#    http://make.paulandlesley.org/autodep.html#traditional for more info.

#DMH Removed -M flag from %.o

 %.o: %.c
	$(VERBOSE)$(NVCC) $(NVCCFLAGS) $(CFLAGS) -c -o $@ $<

 %.o: %.cu
	$(VERBOSE)$(NVCC) $(NVCCFLAGS) $(CFLAGS) -c -o $@ $<


# %.o: %.c
# 	$(VERBOSE)$(NVCC) $(NVCCFLAGS) -M -o $*.P $< && \
# 	cp $*.P $*.d && \
# 	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
# 		  -e '/^$$/ d' -e 's/$$/ :/' < $*.P >> $*.d && \
# 	rm -f $*.P
# 	$(NVCC) $(NVCCFLAGS) $(CFLAGS) -c -o $@ $<

# %.o: %.cu
# 	$(VERBOSE)$(NVCC) $(NVCCFLAGS) -M -o $*.P $< && \
# 	cp $*.P $*.d && \
# 	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
# 		  -e '/^$$/ d' -e 's/$$/ :/' < $*.P >> $*.d && \
# 	rm -f $*.P
# 	$(NVCC) $(NVCCFLAGS) $(CFLAGS) -c -o $@ $<

install: xgpuinfo $(LIBXGPU)
	mkdir -p $(bindir)
	cp xgpuinfo $(bindir)
	mkdir -p $(includedir)
	cp xgpu.h $(includedir)
	mkdir -p $(libdir)
	cp $(LIBXGPU) $(libdir)

uninstall:
	rm -f $(bindir)/xgpuinfo
	rm -f $(includedir)/xgpu.h
	rm -f $(libdir)/$(LIBXGPU)

tags:
	ctags -R .

show:
	@echo NINPUTS=`expr 2 '*' $(NSTATION)`
	@echo NSTATION=$(NSTATION)
	@echo NFREQUENCY=$(NFREQUENCY)
	@echo NTIME=$(NTIME)
	@echo NTIME_PIPE=$(NTIME_PIPE)
	@echo SHARED_ATOMIC_SIZE=$(SHARED_ATOMIC_SIZE)
	@echo COMPLEX_BLOCK_SIZE=$(COMPLEX_BLOCK_SIZE)

clean:
	rm -f cuda_correlator libxgpu.so xgpuinfo
	rm -f $(BINXGPU_OBJS) $(LIBXGPU_OBJS)
	rm -f $(DEPS)
	rm -f tags
	rm -f gpu_monitor*out

.PHONY : all objs tags clean install uninstall show
