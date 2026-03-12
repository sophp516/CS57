; ModuleID = 'miniC_module'
source_filename = "miniC_module"

declare i32 @read()

declare void @print(i32)

define i32 @func(i32 %0) {
entry:
  %n = alloca i32, align 4
  store i32 %0, ptr %n, align 4
  %prod = alloca i32, align 4
  %i = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 1, ptr %n, align 4
  store i32 1, ptr %n, align 4
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
  %i4 = load i32, ptr %n, align 4
  %add = add i32 %i4, 1
  store i32 %add, ptr %n, align 4
  %prod5 = load i32, ptr %n, align 4
  %i6 = load i32, ptr %n, align 4
  %mul = mul i32 %prod5, %prod5
  store i32 %mul, ptr %n, align 4
  br label %while_cond

while_end:                                        ; preds = %while_cond
  %prod7 = load i32, ptr %n, align 4
  store i32 %prod7, ptr %n, align 4
  br label %return_block
}
