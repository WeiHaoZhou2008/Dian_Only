#include <iostream>
#include <fstream>//文件读写
#include <sstream>//字符串流处理
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>//格式化输出
#include <ctime>//时间处理
#include <chrono>

struct Item {//商品
    std::string code;
    std::string name;
    double price;
    int stock;
};

struct CartItem{//购物车
    Item item;
    int num;
};


struct Transaction{
    int serial;//流水号
    std::string date;//日期
    std::string time;//时间
    std::string items_detail;//商品明细字符串
    double total;//总价
};

int next_serial = 1;//全局流水号（重启会从1开始）
std::vector<Transaction> today_sales;//内存中保存当天的交易记录

std::string getCurrentDate(){
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm =*std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

std::string current_business_day = getCurrentDate();//初始化为今天

std::string getCurrentTime(){
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    std::ostringstream oss;
    oss <<  std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

//对商品信息预处理
std::vector<Item> downloadItems(const std::string& filename){
    std::vector<Item> items;
    std::ifstream file(filename);//传入文件流
    if(!file.is_open()){
        std::cerr << "Error: Cannot open " << filename << std::endl;
        return items;//失败则返回空数组
    }
    std::string line;
    std::getline(file,line);//跳过第一行
    while (std::getline(file,line)){
        std::stringstream ss(line);
        std::string code, name, priceStr,stockStr;
        std::getline(ss, code, ',');//用逗号分割数据
        std::getline(ss, name, ',');
        std::getline(ss, priceStr, ',');
        std::getline(ss, stockStr, ',');  // 读取库存字符串
        try {
            double price = std::stod(priceStr);//转换格式
            int stock = 0;
            if (!stockStr.empty()) {
                stock = std::stoi(stockStr);  // 尝试转换为整数
            }
            items.push_back({code, name, price, stock}); // 使用读取到的库存
        }catch (...){
            std::cerr << "Warning: Invalid price in line: " << std::endl;
        }//如果输入非法就报错
    }
    
    return items;
}

//根据条码查找商品
const Item* findItem(const std::vector<Item>& items, const std::string&code) {
    for (const auto& item :items) {
        if (item.code == code){
            return &item;
        }
    }
    return nullptr;
}

Item* findItemMutable(std::vector<Item>& items, const std::string& code) {
    for (auto& item :items) {
        if (item.code == code){
            return &item;
        }
    }
    return nullptr;
}

//欢迎
void Welcome(){
    std::cout << "Welcome to 711 Convenience Store!" << std::endl
              << "1.prices:查看所有商品的名称、条码与价格" << std::endl
              << "2.id:将对应商品加入购物车，并且显⽰价格" << std::endl
              << "3.-id:购物车里对应商品数量-1" << std::endl
              << "4.print:打印现在的⼩票，包含商品种类、数量与价格，以及⼩计价格" << std::endl
              << "5.drop:清空记录，重新开始" << std::endl
              << "6.checkout结账。打印⼩票并清空记录。"  << std::endl
              
              << "7.sales[day],查看当⽇所有销售记录及总营业额，省略 day 参数默认为今天。"<<std::endl
              << "8.admin,进⼊管理员模式（需要输⼊密码)。"<<std::endl
              
              << "9.newday,开始新的⼀天：清空当⽇销售记录"<<std::endl
              << "10.quit或者exit,退出。" << std::endl;
}

void admin_help(){
    std::cout << "Welcome back,administrator! How can I help you?" << std::endl
              << "1.prices:查看所有商品的名称、条码、价格、库存" << std::endl
              << "2.setprice <条码> <新价格> ：修改指定商品的价格。"<<std::endl
              << "3:itemadd <条码> <名称> <价格> ：添加⼀个新商品。" << std::endl
              << "4:itemdel <条码> ：删除⼀个商品。"<< std::endl
              << "5.setstock <条码> <数量> ：直接设置指定商品的库存（⽤于盘点修正）。"<<std::endl
              << "6.restock <条码> <数量> ：增加指定商品的库存。"<<std::endl
              << "7.back ：退出管理员模式，返回收银员模式。"<<std::endl;

}

//split函数
std::vector<std::string> split(const std::string& s){
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string token;
    while(ss >> token){//从ss里读取由空格分隔的片段，存进token里
        tokens.push_back(token);
    }
    return tokens;
}

void prices(std::vector<Item> items){
    std::cout << std::left
              << std::setw(15) << "Items"
              << std::setw(10) << "No."
              << std::setw(10) << "Pri." <<std::endl;
    std::cout << "----------------------------------" <<std::endl;

    for (const auto & item :items){
        std::cout << std::left
                  << std::setw(15) << item.name
                  << std::setw(10) << item.code
                  << std::setw(10) << item.price <<std::endl;
    }
    return;
}

void print(std::vector<CartItem> cart){
    if(cart.empty()){
        std::cout << "Your cart is empty." <<std::endl;
    }
    else{
        std::cout << std::left
                << std::setw(15) << "Item"
                << std::setw(10) << "Num"
                << std::setw(10) << "Subtotal" << std::endl;
        std::cout << "----------------------------------" <<std::endl;

        double total = 0.0;
        for(const auto& sig : cart){
            double subtotal = sig.item.price * sig.num;
            total += subtotal;
            std::cout << std::left
                    << std::setw(15) << sig.item.name
                    << std::setw(10) << sig.num
                    << std::setw(10) << subtotal << std::endl;
            }
        std::cout << "----------------------------------" <<std::endl;
        std::cout << "Total: " << total <<std::endl;
    }
    return;    
}

const std::string ADMIN_PASSWORD = "admin123";

void saveItemsToFile(const std::vector<Item>& items, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot write to " << filename << std::endl;
        return;
    }
    // 写入表头
    file << "code,name,price,stock\n";
    for (const auto& item : items) {
        file << item.code << ","
             << item.name << ","
             << std::fixed << std::setprecision(2) << item.price << "\n";
             << item.stock << "\n";
    }
    file.close();
}


int main(){
    std::vector<Item> items = downloadItems("data/items.csv");
    std::vector<CartItem> cart;//创建购物车

    if(items.empty()){
        std::cerr << "No items loaded.Check data/iems.csv" << std::endl;
        return 1;
    }
    //先来一波 “欢饮光临”
    Welcome();
    bool is_admin = false;

    while(true){//持续输入
        if(is_admin == false){

            std::string input;
            std::cout << ">";
            std::getline(std::cin, input);
            if(input == "quit" || input == "exit") break;//退出

            if(input == "admin"){
                std::cout << "请输入密码："<<std::endl;
                is_admin = true;
                continue;
            }

            if(input == "prices"){//处理prices指令
                prices(items);
                continue;//不再处理后面的指令
            }
            
            //查看购物车,打印小票
            if(input == "print"){
                print(cart);
                continue;
            }

            //清空购物车
            if(input == "drop"){
                cart.clear();
                std::cout << "The cart has been cleared!" <<std::endl;
                continue;
            }

            //结账
            if(input == "checkout"){
                //空购物车
                if(cart.empty()){
                    //强买强卖（bushi
                    //std::cout << "Cart is empty.Cannot checkout." <<std::endl;
                    std::cout << "Your cart is empty.Are you sure to checkout?[Y/N]" <<std::endl;
                    std::string ask;
                    std::getline(std::cin, ask);
                    if(ask == "Y"){
                        std::cout << "You don't need to pay anything." << std::endl
                                << "Welcome to 711 Convenience Store next time!" <<std::endl;
                    }
                    continue;
                }

                bool stock_enough = true;
                std::string error_item;

                // 1. 预检库存
                for (const auto& sig : cart) {
                    Item* item = findItemMutable(items, sig.item.code);
                    if (!item || item->stock < sig.num) {
                        stock_enough = false;
                        error_item = sig.item.name;
                        break;
                    }
                }

                if (!stock_enough) {
                    std::cout << "Checkout failed! Stock insufficient for: " << error_item << "\n";
                    continue; // 拦截结账
                }

                //  扣减库存（事务性操作）
                for (const auto& sig : cart) {
                    Item* item = findItemMutable(items, sig.item.code);
                    if (item) item->stock -= sig.num;
                }


                // 4. 同步库存到文件（防止数据丢失）
                

                //打印小票
                std::cout << "Receipt" <<std::endl;
                double total = 0.0;
                std::cout << std::left
                        << std::setw(15) << "Item"
                        << std::setw(5) << "Pri"
                        << std::setw(6) << "Qty" 
                        << std::setw(8) << "Amount" << std::endl;
                std::cout << "----------------------------------" <<std::endl;
                std::ostringstream items_detail_ss;

                for(const auto& sig :cart){
                    double subtotal = sig.item.price * sig.num;
                    std::cout << std::left
                            << std::setw(15) << sig.item.name
                            << std::setw(5) << sig.item.price
                            << std::setw(3) << " x " 
                            << std::setw(3) << sig.num 
                            << std::setw(3) << " = "
                            << std::setw(5) << subtotal << std::endl;
                    total += subtotal;
                    items_detail_ss << sig.item.name << " x" << sig.num << "<br>";
                }
                std::cout << "----------------------------------" <<std::endl;
                std::cout << "Checkout successfully! Total amount: " << total <<std::endl;
                
                //创建交易记录
                Transaction trans;
                trans.serial = next_serial++;
                trans.date = current_business_day;
                trans.time = getCurrentTime();
                trans.items_detail = items_detail_ss.str();
                trans.total = total;

                //存入内存
                today_sales.push_back(trans);

                //文件持久化：追加写入
                std::ofstream outfile("sales.csv", std::ios::app);
                if (outfile.is_open()) {
                    outfile << trans.serial << "," 
                            << trans.date << "," 
                            << trans.time << "," 
                            << trans.items_detail << "," 
                            << std::fixed << std::setprecision(2) << trans.total << "\n";
                    outfile.close();
                    saveItemsToFile(items, "data/items.csv");
                    std::cout << "Record saved. Stock updated.\n";
                }
                else{
                    std::cout << "Error: Unable to open sales.csv for writing." << std::endl;
                }
                cart.clear();
                continue;
            }

            if(input.substr(0,5) == "sales"){
                std::string target_date;
                if(input.length()>6){
                    target_date = input.substr(6);//获取指定日期
                }
                else{
                    target_date = current_business_day;//默认今天
                }

                //从文件读取并筛选
                std::ifstream infile("sales.csv");
                std::vector<Transaction> records;

                if(infile.is_open()){
                    std::string line;
                    while(std::getline(infile,line)){
                        std::stringstream ss(line);
                        std::string field;
                        Transaction rec;

                        std::getline(ss,field,',');
                        try{
                            rec.serial = std::stoi(field);
                        }catch(...){
                            continue;
                        }
                        std::getline(ss,rec.date,',');
                        std::getline(ss,rec.time,',');
                        std::getline(ss, rec.items_detail, ',');
                        std::getline(ss, field, ','); 
                        try{
                            rec.total = std::stod(field);
                        }catch(...){
                            continue;
                        }

                        //把占位符<br>替换为换行符\n
                        size_t pos = 0;
                        while ((pos = rec.items_detail.find("<br>", pos)) != std::string::npos) {
                            rec.items_detail.replace(pos, 4, "\n");
                            pos += 1;
                        }

                        if(rec.date == target_date){
                            records.push_back(rec);
                        }
                    }
                    infile.close();
                }

                // 格式化输出
                std::cout << "Date: " << target_date << std::endl;
                std::cout << std::left << std::setw(6) << "No." 
                          << std::setw(10) << "Time" 
                          << std::setw(20) << "Items" 
                          << std::setw(10) << "Amount" << "\n";
                std::cout << std::string(40, '-') << "\n";
                
                double daily_total = 0.0;
                for (const auto& rec : records) {
                    daily_total += rec.total;
                    
                    // 按换行符拆分明细并逐行输出
                    std::stringstream detail_ss(rec.items_detail);
                    std::string line;
                    bool first_line = true;
                    
                    while (std::getline(detail_ss, line, '\n')) {
                        if (first_line) {
                            std::cout << std::left << std::setw(6) << rec.serial 
                                    << std::setw(10) << rec.time 
                                    << line << "\n";
                            first_line = false;
                        } else {
                            // 后续行对齐 Items 列
                            std::cout << std::left << std::setw(16) << "" << line << "\n";
                        }
                    }
                    
                    // 输出该行的总价
                    std::cout << std::right << std::setw(36) << std::fixed << std::setprecision(2) << rec.total << "\n";
                }
                std::cout << std::string(40, '-') << "\n";
                std::cout << "Daily: " << std::fixed << std::setprecision(2) << daily_total << std::endl;
                continue;
            }

            if (input == "newday") {
                // 将 current_business_day 设为明天
                std::tm tm = {};
                std::istringstream iss(current_business_day);
                iss >> std::get_time(&tm, "%Y-%m-%d");
                tm.tm_mday += 1;  // 加一天
                std::mktime(&tm); // 规范化（处理跨月跨年）
                std::ostringstream oss;
                oss << std::put_time(&tm, "%Y-%m-%d");
                current_business_day = oss.str();

                today_sales.clear();
                std::cout << "New business day: " << current_business_day << std::endl;
                continue;
            }

            //处理多个商品条码的输入（包括删除）
            //分裂
            std::vector<std::string> tokens = split(input);

            for (const std::string& token : tokens){
                int isAdd = 1;//判断是加还是减
                std::string code = token;
                if(!code.empty() && code[0] == '-'){
                    isAdd = -1;
                    code = code.substr(1);
                }
                const Item* item = findItem(items,code);//查找输入的条码
                if(item == nullptr){
                    std::cout << "ERROR: code "<< code << " not found" << std::endl;
                    continue;//找不到就报错
                }
                else{
                    //先检查是否在购物车里
                    bool foundInCart = false;
                    CartItem eee;
                    for(auto& sig : cart){
                        if(sig.item.code == code){
                            foundInCart = true;
                            eee = sig;
                            break;
                        }
                    }
                    if(isAdd == 1){
                        //如果是添加
                        if(!foundInCart){
                            cart.push_back({*item,1});
                            eee.num = 1;
                            eee.item.price = item->price;
                            eee.item.name = item->name;
                            eee.item.code = item->code;
                        }//没有就新增
                        else for(auto& sig : cart){
                            if(sig.item.code == code){
                                sig.num++;
                                eee.item=sig.item;
                                eee.num = sig.num;
                                break;
                            }
                        }
                        //正常输出
                        std::cout <<"Added " << item->name << " to cart." << std::endl;
                        double PRI = eee.num * eee.item.price;
                        std::cout << std::left
                                << std::setw(15) << item->name
                                << std::setw(5) << eee.item.price
                                << std::setw(3) << " * "
                                << std::setw(3) << eee.num 
                                << std::setw(3) << " = "
                                << std::setw(5) << PRI << std::endl;
                    }
                    else if(isAdd == -1){
                        if(!foundInCart){
                            std::cout << "ERROR: " <<eee.item.name << " is not in your cart."<<std::endl;
                            continue;
                        }//如果本来就没有，报错

                        //如果有，在cart里面找一下，减1
                        bool ifIn = true;//判断减1之后是否还在cart
                        for (auto& ci : cart){
                            if(ci.item.code == code){
                                ci.num --;
                                if(ci.num<= 0){
                                    ifIn = false;
                                }
                                eee = ci;
                                break;
                            }
                        }

                        if(ifIn == false){
                            for (auto it = cart.begin(); it != cart.end(); ++it) {
                                if (it->item.code == code) {
                                    if (it->num == 0){
                                        cart.erase(it);
                                    }
                                    break;
                                }
                            }
                            std::cout <<"Removed " << item->name << " from cart." << std::endl;
                            continue;
                        }

                        //减1之后还在，正常输出

                        std::cout <<"Subtracted " << item->name << " from cart." << std::endl;
                        double PRI = eee.num * eee.item.price;
                        std::cout << std::left
                                << std::setw(15) << item->name
                                << std::setw(5) << eee.item.price
                                << std::setw(3) << " * "
                                << std::setw(3) << eee.num 
                                << std::setw(3) << " = "
                                << std::setw(5) << PRI << std::endl;
                    }
                }
            }
        }
        else{//管理员模式
            std::string assure;
            std::getline(std::cin, assure);
            if(assure != ADMIN_PASSWORD){
                std::cout << "Password wrong!"<< std::endl;
                is_admin = false;
                continue;
            }
            admin_help();
            std::string input_admin;
            while(true){//持续输入
                std::cout << "admin>";
                std::getline(std::cin, input_admin);
                std::vector<std::string> admin_order = split(input_admin);
                std::string mode = admin_order[0];

                if(mode == "setprice"&&admin_order.size() == 3){
                    std::string code = admin_order[1];
                    double newPrice = stod(admin_order[2]);
                    Item* item = findItemMutable(items,code);
                    if(item != nullptr){
                        item->price = newPrice;
                        std::cout << "Price updated."<<std::endl;
                    }
                    else{
                        std::cout << "ERROR: Item not found."<<std::endl;
                    }
                    saveItemsToFile(items, "data/items.csv");
                    continue;
                }

                if(mode == "itemdel"&&admin_order.size() == 2 ){
                    std::string code = admin_order[1];
                    for(auto it = items.begin();it != items.end();++it){
                        if(it->code == code){
                            std::cout << it->name << "(" << code <<") removed."<<std::endl;
                            items.erase(it);
                            break;
                        }
                    }
                    saveItemsToFile(items, "data/items.csv");
                    continue;
                }
                if (mode == "itemadd" && admin_order.size() == 4) {
                    std::string code = admin_order[1];
                    std::string name = admin_order[2];
                    double price = std::stod(admin_order[3]);
                    // 检查条码是否已存在
                    if (findItem(items, code) != nullptr) {
                        std::cout << "ERROR: Code already exists." << std::endl;
                    } else {
                        items.push_back({code, name, price, 0}); // stock 默认为 0
                        saveItemsToFile(items, "data/items.csv");
                        std::cout << "Item added." << std::endl;
                    }
                    continue;
                }

                // 查看商品信息（包含库存）
                if (mode == "prices") {
                    std::cout << std::left << std::setw(10) << "Item" 
                            << std::setw(6) << "No." 
                            << std::setw(8) << "Pri." 
                            << std::setw(6) << "Stock" << "\n";
                    std::cout << std::string(30, '-') << "\n";
                    for (const auto& item : items) {
                        std::cout << std::left << std::setw(10) << item.name 
                                << std::setw(6) << item.code 
                                << std::setw(8) << std::fixed << std::setprecision(2) << item.price 
                                << std::setw(6) << item.stock << "\n";
                    }
                    continue;
                }

                // 进货（增加库存）
                if (mode == "restock" && admin_order.size() == 3) {
                    Item* item = findItemMutable(items, admin_order[1]);
                    if (item) {
                        int add_qty = std::stoi(admin_order[2]);
                        item->stock += add_qty;
                        saveItemsToFile(items, "data/items.csv"); // 持久化[1](@ref)
                        std::cout << "Restocked. New stock: " << item->stock << "\n";
                    } else {
                        std::cout << "Item not found.\n";
                    }
                    continue;
                }

                // 盘点（直接设置库存）
                if (mode == "setstock" && admin_order.size() == 3) {
                    Item* item = findItemMutable(items, admin_order[1]);
                    if (item) {
                        int new_qty = std::stoi(admin_order[2]);
                        item->stock = new_qty;
                        saveItemsToFile(items, "data/items.csv"); // 持久化[1](@ref)
                        std::cout << "Stock updated. New stock: " << item->stock << "\n";
                    } else {
                        std::cout << "Item not found.\n";
                    }
                    continue;
                }
                if(input_admin == "back"){
                    is_admin = false;
                    break;
                }
            }
        }
        
    }
    return 0;
}