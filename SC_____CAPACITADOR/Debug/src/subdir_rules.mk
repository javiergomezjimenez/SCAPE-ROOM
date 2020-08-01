################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
src/%.obj: ../src/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccs920/ccs/tools/compiler/ti-cgt-msp430_18.12.3.LTS/bin/cl430" -vmsp -O0 --use_hw_mpy=none --include_path="C:/ti/ccs920/ccs/ccs_base/msp430/include" --include_path="C:/ti/ccs920/ccs/ccs_base/msp430/include" --include_path="D:/usuario/REPO/SCAPE-ROOM/SC_____CAPACITADOR" --include_path="C:/ti/ccs920/ccs/tools/compiler/ti-cgt-msp430_18.12.3.LTS/include" --advice:power=all --define=__MSP430G2553__ -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/$(basename $(<F)).d_raw" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


