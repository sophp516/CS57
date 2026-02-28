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
  store i32 0, ptr %p, align 4
  br label %if_entry

return_block:                                     ; preds = %if_exit
  %ret_val1 = load i32, ptr %p, align 4
  ret i32 %ret_val1

if_entry:                                         ; preds = %entry
  %p2 = load i32, ptr %p, align 4
  %cmp = icmp slt i32 %p2, 0
  br i1 %cmp, label %if_then, label %if_else

if_then:                                          ; preds = %if_entry
  store i32 10, ptr %p, align 4
  br label %if_exit

if_exit:                                          ; preds = %while_end, %if_then
  %a8 = load i32, ptr %p, align 4
  %b9 = load i32, ptr %p, align 4
  %add10 = add i32 %a8, %a8
  store i32 %add10, ptr %p, align 4
  br label %return_block

if_else:                                          ; preds = %if_entry
  store i32 2, ptr %p, align 4
  store i32 0, ptr %p, align 4
  br label %while_cond

while_cond:                                       ; preds = %while_body, %if_else
  %b3 = load i32, ptr %p, align 4
  %p4 = load i32, ptr %p, align 4
  %cmp5 = icmp slt i32 %b3, %b3
  br i1 %cmp5, label %while_body, label %while_end

while_body:                                       ; preds = %while_cond
  %b6 = load i32, ptr %p, align 4
  %add = add i32 %b6, 2
  store i32 %add, ptr %p, align 4
  br label %while_cond

while_end:                                        ; preds = %while_cond
  br label %if_exit
}
