open System

let opcodes =
    [
      "brk";   // software breakpoint 
      "pop";   // throw away top item
      "swap";  // switch the top two items of the stack
      "swapn";  // switch 2 items of the stack as determined by
              // stack-x and stack-y where x and y are the upper and lower
              // 16 bits of the operand
      "dup";   // copy the top item of the stack
      "ldval";
      "ldvals";
      //      "ldvalf";
      "ldvalb";
      "ldvar";
      "stvar";
      "p_stvar";
      "rvar"; 
      "ldprop"; // accept game obect or location ref
      "p_ldprop";
      "stprop";
      "p_stprop";
      "inc";
      "dec";
      "neg";
      "add";
      "sub";
      "mul";
      "div";      
      "mod";
      "pow";
      "tostr";
      "toint";
      "rndi";      

      "startswith";
      "p_startswith";
      "endswith";
      "p_endswith";
      "contains";
      "p_contains";
      "indexof";
      "p_indexof";
      "substring";
      "p_substring";
      
      "ceq";
      "cne";
      "cgt";
      "cgte";
      "clt";
      "clte";
      "beq";
      "bne";
      "bgt";
      "blt"; // ho ho ho
      "bt";
      "bf";
      "branch";

      "isobj";
      "isint";
      "isbool";
      "isloc";
      "islist";
      
      // AA and list ops
      "createobj";
      "cloneobj";
      "getobj";
      "getobjs";
      "delprop";
      "p_delprop";
      "delobj";
      "moveobj"; // change parents
      "p_moveobj";
      "createarr";
      "appendarr";
      "p_appendarr";
      "prependarr";
      "p_prependarr";
      "removearr";
      "p_removearr";
      "len";
      "p_len";
      "index";
      "p_index";
      "setindex";
      
      "keys";
      "values";
      "syncprop";
      
      "getloc";  // accepts location ref or string -> Locationref
      "genloc";
      "genlocref";
      "setlocsibling";  // rel :: host :: locref
      "p_setlocsibling"; 
      "setlocchild";
      "p_setlocchild";
      "setlocparent";
      "p_setlocparent";
      // these gets return LocationReferences
      "getlocsiblings";
      "p_getlocsiblings";
      "getlocchildren";
      "p_getlocchildren";
      "getlocparent";
      "p_getlocparent";

      //todo: probably need ability to remove locations and links ...      
      "setvis";
      "p_setvis";
      "adduni";
      "deluni";
                       
      "splitat";
      "shuffle";
      "sort"; // number / string lists
      "sortby"; // prop name
      
      // flowroutine / messaging
      "genreq";
      "addaction";
      "p_addaction";

      "suspend";
      "cut";
      "say";

      "pushscope";
      "popscope";
      "lambda";
      "apply";
      "ret";

      "dbg";  // prints to the console
      "dbgl"
      "getraw"
      "fork"
      "join"
      "cons"
      "head"
      "tail"
      "islist"
      "createlist"
      "isfunc" 
    ]

let extended = 
    Set.ofList
        [
            "swapn"
            "ldval"
            "ldvals"
            "ldvalb"
            "ldvar"
            "stvar"
            "p_stvar"
            "rvar"
            "beq"
            "bne"
            "bgt"
            "blt"
            "bt"
            "bf"
            "branch"
            "lambda"
        ]

let genEnum() = 
    let sb = System.Text.StringBuilder()
    let (~~) (s:string) = sb.Append(s) |> ignore
    let (~~~) (s:string) = sb.AppendLine(s) |> ignore
    ~~~ "enum opcode"
    ~~~ "\t{"
    opcodes
    |> List.iteri(fun i s ->
                  ~~~ (sprintf "\t\t%s = %i%s" s i (if i = opcodes.Length - 1 then "" else ","))
                  )
    
    ~~~ "\t};"
    printf "%s" <| sb.ToString()


let genc() =
    let sb = System.Text.StringBuilder()
    let (~~) (s:string) = sb.Append(s) |> ignore
    let (~~~) (s:string) = sb.AppendLine(s) |> ignore
    ~~~ "  switch(o)"
    ~~~ "    {"    
    opcodes
    |> List.iteri(fun i s ->
                  ~~~ (sprintf "    case %s:" s)
                  ~~~ (sprintf "      DL(\"%s Not implemented\\n\")" s)
                  ~~~ "      break;"
                  )
    
    ~~~ "    }"
   
    printf "%s" (sb.ToString())
//    sb.ToString()

let genCJsonOpcodes() = 
    let sb = System.Text.StringBuilder()
    let (~~) (s:string) = sb.Append(s) |> ignore
    let (~~~) (s:string) = sb.AppendLine(s) |> ignore
    opcodes
    |> List.iteri(fun i s -> 
        ~~~ "opcode = hash_init(2);"
        ~~~ (sprintf "hash_set(opcode, ra_init_str(\"code\"), int_to_memref(%i));" i)
        ~~~ (sprintf "hash_set(opcode, ra_init_str(\"extended\"), int_to_memref(%i));" (if Set.contains s extended then 1 else 0) )
        ~~~ (sprintf "hash_set(opcodes, ra_init_str(\"%s\"), opcode);" s)
    )
    
    
    printf "%s" <| sb.ToString()

genc()  

genEnum()

genCJsonOpcodes() 