#!/bin/bash

directory_tree()
{
    cd "$1"                                      #change directory to $1
    for f in *                                   #for each file/folder in dir, name in f
    do
        if ! [ "$f" = "$output_dir" ];
        then
            if [ -d "$f" ]; then                     #if folder                      #increase lvl
            #print_in_level $2 "$f"                   #print folder name
            directory_tree "$f"          #call the folder as root recursively
            elif [ -f "$f" ];                          #if file
            then   

                filename=$(basename -- "$f")
                extension="${filename##*.}"

                case `basename "$filename"` in
                *.* )  
                    if ! is_ignored $extension 
                    then
                        copy_and_insert_path "$f" "$extension" "$PWD/$f"
                        #print_in_level $2 "$f"               #print filename
                    else
                        ignored_files_count=$((ignored_files_count+1))
                    fi  
                    ;;
                * )
                    copy_and_insert_path "$f" "others" "$PWD/$f"

                    ;;
                esac      
            fi
        fi
    done
    cd ../
}

copy_and_insert_path()
{
    myfile="$1"
    extension="$2"

    if ! [ -d "$output_dir/$extension" ]; then 
        mkdir "$output_dir/$extension"
    fi
    if ! [ -f "$output_dir/$extension/$myfile" ]; then 
        cp "./$myfile" "$output_dir/$extension"
        file_dir="$3"
        relative_path=$(realpath --relative-to="$script_path" "$file_dir")
        des_file="desc_${extension}.txt"

        if ! [ -f "$output_dir/$extension/$des_file" ]; then 
            echo "$relative_path" > "$output_dir/$extension/$des_file"
        else
            echo "$relative_path" >> "$output_dir/$extension/$des_file"
            
        fi
    fi    
}

read_file()
{
    filename="$1"
    n=0
    #while read line || [ -n "$line" ]; do
    while IFS=$'\r\n' read -r line || [ -n "$line" ]; do
    #while read line; do
        ARRAY[$n]=$line
        n=$((n+1))
    done < "$filename"
    echo ${ARRAY[*]}
}

is_ignored()
{
    file_ext="$1"
    
    for str in ${ARRAY[@]}; do   
        if [ "$file_ext" = "$str" ];
        then
            return 0
        fi
    done
    return 1

}

make_csv_file()
{
    echo "file_type, no_of_files">"$csv_file_path"
    cd "$output_dir"
    for f in *  
    do
        count_files=$(find ./$f -maxdepth 1 -type f | wc -l)
        count_files=$((count_files-1))
        echo "$f, $count_files">>"$csv_file_path"
    done
    echo "ignored, $ignored_files_count">>"$csv_file_path"
    
}


working_dir=""
output_dir="$PWD/1705030_output"
csv_file_path="$PWD/1705030.csv"
ignored_files_count=0
script_path="$PWD"


get_working_dir()
{
    curr_dir="$PWD"
    cd "$1"
    working_dir="$PWD"
    cd "$curr_dir"
    output_dir="$(dirname "$working_dir")/1705030_output"
    csv_file_path="$(dirname "$working_dir")/1705030.csv"

    #output_dir="$(dirname "$script_path")/1705030_output"
    #csv_file_path="$(dirname "$script_path")/1705030.csv"
    if ! [ -d "$output_dir" ]; then 
        mkdir -p "$output_dir"
    fi
}



input_file_name=""

if [ $# -eq 2 ]; then       #both directory and input file
    if ! [ -f "$2" ]; then 
        echo "Invalid input file"
        echo "Please give a valid input file name: "
        read input_file_name
        while ! [ -f "$input_file_name" ]; do
            echo "Please give a valid input file name: "
            read input_file_name
        done
    else
        input_file_name="$2"
    fi

    get_working_dir "$1"
    read_file "$input_file_name"
    directory_tree "$1"
    make_csv_file

elif [ $# -eq 1 ]; then     #only input file
    if ! [ -f "$1" ]; then 
        echo "Invalid input file"
        echo "Please give a valid input file name: "
        read input_file_name
        while ! [ -f "$input_file_name" ]; do
            echo "Please give a valid input file name: "
            read input_file_name
        done
    else
        input_file_name="$1"
    fi    
    get_working_dir "."
    read_file "$input_file_name"
    directory_tree "."
    make_csv_file
else
    echo "Invalid command"
    echo "To use this script:"
    echo "1.    Give the working directory name (optional) and input file name as a command-line argument"
    echo "2.    If any working directory name is not provided, the script is lying in the root working directory"
fi






