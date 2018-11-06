augroup filetype
    autocmd! BufRead,BufNewFile BUILD set filetype=blade
augroup end

set nocompatible " 关闭 vi 兼容模式
syntax on " 自动语法高亮
set number " 显示行号
set cursorline " 突出显示当前行
set ruler " 打开状态栏标尺
set shiftwidth=2 " 设定  和  命令移动时的宽度为 4
set softtabstop=2 " 使得按退格键时可以一次删掉 4 个空格
set tabstop=2 " 设定 tab 长度为 4
set expandtab
set nobackup " 覆盖文件时不备份
set autochdir " 自动切换当前目录为当前文件所在的目录
set backupcopy=yes " 设置备份时的行为为覆盖
set ignorecase smartcase " 搜索时忽略大小写，但在有一个或以上大写字母时仍保持对大小写敏感
set nowrapscan " 禁止在搜索到文件两端时重新搜索
set incsearch " 输入搜索内容时就显示搜索结果
set hlsearch " 搜索时高亮显示被找到的文本
set noerrorbells " 关闭错误信息响铃
set novisualbell " 关闭使用可视响铃代替呼叫
set t_vb= " 置空错误铃声的终端代码
set magic " 设置魔术
set hidden " 允许在有未保存的修改时切换缓冲区，此时的修改由 vim 负责保存
set guioptions-=T " 隐藏工具栏
set guioptions-=m " 隐藏菜单栏
set smartindent " 开启新行时使用智能自动缩进
set backspace=indent,eol,start
set cmdheight=1 " 设定命令行的行数为 1
set laststatus=2 " 显示状态栏 默认值为 1, 无法显示状态栏
set foldenable " 开始折叠
"set foldmethod=syntax " 设置语法折叠
set foldcolumn=0 " 设置折叠区域的宽度
setlocal foldlevel=1
set encoding=utf-8
set termencoding=utf-8
set formatoptions+=mM
set fencs=utf-8

let Tlist_Show_One_File=1     "不同时显示多个文件的tag，只显示当前文件的    
let Tlist_Exit_OnlyWindow=1   "如果taglist窗口是最后一个窗口，则退出vim   


set nocompatible              " be iMproved, required
filetype off                  " required

" set the runtime path to include Vundle and initialize
set rtp+=~/.vim/bundle/Vundle.vim
call vundle#begin()

" let Vundle manage Vundle, required
Plugin 'gmarik/Vundle.vim'
Plugin 'fatih/vim-go'

" All of your Plugins must be added before the following line
call vundle#end()            " required
filetype plugin indent on    " required


"###################    set file head start  #########################
""autocmd创建新文件自动调用setfilehead()函数
autocmd BufNewFile *.v,*.sv,*.cpp,*.c,*.cc,*.h exec ":call Setfilehead()"
func Setfilehead()
  call append(0,'/***********************************************')
  call append(1,'Copyright           : 2015 youme.im Inc.') 
  call append(2,'Filename            : '.expand("%"))
  call append(3,'Author              : leef@youme.im')
  call append(4,'Description         : ---')
  call append(5,'Create              : '.strftime("%Y-%m-%d %H:%M:%S"))
  call append(6,'Last Modified       : '.strftime("%Y-%m-%d %H:%M:%S"))
  call append(7,'***********************************************/')
endfunc       

