

# 背景

对于一个系统来说，错误码是很重要的，好的错误码能够帮助使用者快速了解出错的原因，，同时也能帮助系统维护者快速定位问题以及快速解决。

目前Cyprestore的错误码定义存在如下问题：

* 错误码没有分号段
* 错误码较少，想不出恰当的错误码定义时只能使用通用的错误码
* 客户端和服务端单独定义不同的错误码，客户端定义负数错误码，服务端定义正数错误码，格式不统一
* 错误码和protobuf的错误码需要相互转换

# 业界方案

业界定义的错误码通常分为3种类型：纯数字错误码、纯字母错误码、字母+数字错误码。这3种使用方式各有优缺点，纯数字错误码不利于记忆和分类；纯字母错误码不利于排序以及非英语母语者准确描述；字母+数字错误码为字符串类型，实现不如纯字母优雅。

好的错误码标准：

* 好的错误码能够快速知晓错误来源
* 提供友好的提示信息
* 统一定义，方便搜索
* 错误码不应体现版本号和错误等级信息，新增错误码直接递增

# 错误码设计示例

## 字母+数字错误码设计（阿里巴巴《Java开发手册》）

第一个字母代表错误来源，如A表示用户错误

|错误码           |中文描述            |说明            |
|---             |---                |----           |
|00000           |一切ok             |正确执行后的返回  |
|A0001           |用户端错误          |一级宏观错误码   |
|A0100           |用户注册错误         |二级宏观错误码  |
|A0101           |用户未同意隐私协议    |
|A0102           |注册国际或地区受限    |
|A0110           |用户名校验失败       |
|A0111           |用户名已存在         |


## 纯字母错误码设计（阿里云API错误中心）

ErrorCode具有一定自解释能力

|Error Code                             |Error Message            |描述            |
|---                                    |---                      |----           |
|RealNameAuthenticationError            |Your account has not passed the real-name authentication yet.             |您的账户尚未通过实名认证，请先实名认证后再操作  |
|InvalidAccountStatus.NotEnoughBalance  |Your account does not have enought balance.                        |账号余额不足，请您先充值再进行该操作。    |
|IncorrectVSwitchStatus                 |VSwitch Creation simultaneously is not supported.                      |创建交换机失败，VPC中有交换机的状态为Creating。  |
|QutoExceeded.Vpc                       |VPC quota exceeded.        |用户名下的VPC数量达到配额上限，请提交工单申请提高配额。|


## 纯数字错误码设计（Google API Design Guide）

类似Restful错误码设计，200表示成功

|HTTP            |RPC                  |消息实例            |
|---             |---                  |----           |
|400             |INVALID_ARGUMENT     |Request field x.y.z is xxx, expected one of [yyy,zzz] |
|400             |FAILED_PRECONDITION  |Resource xxx is a non-empty directory, so it cannot be deleted.   |
|400             |OUT_OF_RANGE         |Parameter 'age' is out of range [0, 125] |
|401             |UNAUTHENTICATED      |Invalid authentication credentials.
|403             |PERMISSION_DENIED    |Permission 'xxx' denied on file 'yyy'
|404             |NOT_FOUND            |Resource 'xxx' not found
|409             |ABORTED              |Couldn't acquire lock on resource 'xxx'

# Cyprestore错误码设计

设计原则
1. 使用纯数字错误码，且使用负数，0表示成功，使用const int常量；
2. 分号段设计，各模块按照分段申请错误码，各模块中可以再进行号段划分。
   - -1 ~ -999 预留
   - -1000 ~ -1999 表示公共错误，如参数错误等
   - -2000~-2999表示客户端SDK(libcypre)错误
   - -3000~-3999表示ExtentManager错误
   - -4000~-4999表示ExtentServer错误
   - -5000~-5999表示SetManager错误
   - -6000~-6999表示Access错误
3. 变量名规范
   - 客户端SDK：CYPRE_C_XXX
   - ExtentManager：CYPRE_EM_XXX
   - ExtentServer：CYPRE_ES_XXX
   - SetManager：CYPRE_SM_XXX
   - Access：CYPRE_AC_XXX
   - 公共：CYPRE_ER_XXX
4. 错误码解释由message来详细说明

示例：

|Error Code           |Error Message            |中文描述
|---                  |---                      |----
|0                    |OK                       |成功
|-1000                |invalid parameter        |参数错误
|-1001                |time out                 |请求超时
|-1002                |net error                |网络错误
|---                  |
|-2000                |stream corrupt           |数据流错误
|-2001                |data too large           |数据块超过限制
|---                  |
|-3000                |pool not found           |存储池不存在
|-3001                |blob not found           |Blob不存在
