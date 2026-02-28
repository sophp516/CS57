; ModuleID = 'miniC_module'
source_filename = "miniC_module"

declare i32 @read()

declare void @print(i32)

define i32 @func(i32 %0) {
entry:
  %p = alloca i32, align 4
  store i32 %0, ptr %p, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 10, ptr %p, align 4
  %p3 = load i32, ptr %p, align 4
  %add = add i32 10, %p3
  store i32 %add, ptr %p, align 4
  %b4 = load i32, ptr %p, align 4
  store i32 %b4, ptr %p, align 4
  br label %return_block

return_block:                                     ; preds = %entry
  %ret_val1 = load i32, ptr %p, align 4
  ret i32 %ret_val1
}
