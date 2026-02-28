; ModuleID = 'miniC_module'
source_filename = "miniC_module"

declare i32 @read()

declare void @print(i32)

define i32 @func(i32 %0) {
entry:
  %n = alloca i32, align 4
  store i32 %0, ptr %n, align 4
  %sum = alloca i32, align 4
  %i = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 0, ptr %n, align 4
  store i32 0, ptr %n, align 4
  br label %if_entry

return_block:                                     ; preds = %if_exit, %if_then
  ret i32 0

if_entry:                                         ; preds = %entry
  %n2 = load i32, ptr %n, align 4
  %cmp = icmp sle i32 %n2, 0
  br i1 %cmp, label %if_then, label %if_else

if_then:                                          ; preds = %if_entry
  store i32 0, ptr %n, align 4
  br label %return_block
  br label %if_exit

if_exit:                                          ; preds = %while_end, %if_then
  store i32 0, ptr %n, align 4
  br label %return_block

if_else:                                          ; preds = %if_entry
  br label %while_cond

while_cond:                                       ; preds = %while_body, %if_else
  %i3 = load i32, ptr %n, align 4
  %n4 = load i32, ptr %n, align 4
  %cmp5 = icmp slt i32 %i3, %i3
  br i1 %cmp5, label %while_body, label %while_end

while_body:                                       ; preds = %while_cond
  %i6 = load i32, ptr %n, align 4
  %add = add i32 %i6, 1
  store i32 %add, ptr %n, align 4
  br label %while_cond

while_end:                                        ; preds = %while_cond
  br label %if_exit
}
