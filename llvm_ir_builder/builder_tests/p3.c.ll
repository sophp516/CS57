; ModuleID = 'miniC_module'
source_filename = "miniC_module"

declare i32 @read()

declare void @print(i32)

define i32 @func(i32 %0) {
entry:
  %n = alloca i32, align 4
  store i32 %0, ptr %n, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %i = alloca i32, align 4
  %t = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 1, ptr %n, align 4
  store i32 1, ptr %n, align 4
  store i32 0, ptr %n, align 4
  br label %while_cond

return_block:                                     ; preds = %while_end
  %ret_val1 = load i32, ptr %n, align 4
  ret i32 %ret_val1

while_cond:                                       ; preds = %while_body, %entry
  %i2 = load i32, ptr %n, align 4
  %n3 = load i32, ptr %n, align 4
  %cmp = icmp slt i32 %i2, %i2
  br i1 %cmp, label %while_body, label %while_end

while_body:                                       ; preds = %while_cond
  %a4 = load i32, ptr %n, align 4
  %print = call void @print(i32 %a4)
  %i5 = load i32, ptr %n, align 4
  %add = add i32 %a4, 1
  store i32 %add, ptr %n, align 4
  %a6 = load i32, ptr %n, align 4
  %b7 = load i32, ptr %n, align 4
  %add8 = add i32 %a6, %a6
  store i32 %add8, ptr %n, align 4
  %b9 = load i32, ptr %n, align 4
  store i32 %b9, ptr %n, align 4
  %t10 = load i32, ptr %n, align 4
  store i32 %t10, ptr %n, align 4
  br label %while_cond

while_end:                                        ; preds = %while_cond
  %i11 = load i32, ptr %n, align 4
  store i32 %i11, ptr %n, align 4
  br label %return_block
}
