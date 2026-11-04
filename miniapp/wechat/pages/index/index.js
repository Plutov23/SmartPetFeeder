const config = require('../../config')

Page({
  data: {
    temperature: 0,
    humidity: 0,
    foodLevel: 0,
    currentTime: '00:00',
    petLabel: '', 
    videoSrc: '',

    activeTab: 'home',
    showRecords: false,
    showDevice: false,
    isFeeding: false,
    showSuccess: false,

    showScheduleModal: false,
    editingSchedule: null,
    tempTime: '08:00',
    tempAmount: 30,

    socketTask: null, 
    pollingTimer: null,
    clockTimer: null,

    todayTotal: 0, 
    weekTotal: 0,
    foodieTitle: '健康干饭',
    todayRecords: [],

    schedules: [
      { id: 0, time: '08:00', amount: 30, frequency: '每天', enabled: true }, 
      { id: 1, time: '18:30', amount: 50, frequency: '每天', enabled: false }
    ]
  },

  onLoad() {
    this.updateClock();
    this.data.clockTimer = setInterval(() => this.updateClock(), 1000);
    this.fetchCloudData();
    // 每 5 秒去云端拉一次数据
    this.data.pollingTimer = setInterval(() => this.fetchCloudData(), 5000);
    this.reconnectCamera();
    
    this.loadFeedingRecords();
  },

  onUnload() {
    if (this.data.clockTimer) clearInterval(this.data.clockTimer);
    if (this.data.pollingTimer) clearInterval(this.data.pollingTimer);
    if (this.data.socketTask) this.data.socketTask.close();
  },

  loadFeedingRecords() {
    const now = new Date();
    const todayStr = `${now.getFullYear()}-${now.getMonth()+1}-${now.getDate()}`;
    
    let logs = wx.getStorageSync('feed_logs');
    if (!logs || logs.length === 0) {
      logs = [
        { id: Date.now() - 10000, date: todayStr, time: '08:00', type: '定时喂食', amount: 30 },
        { id: Date.now() - 5000, date: todayStr, time: '12:30', type: '定时喂食', amount: 30 }
      ];
      wx.setStorageSync('feed_logs', logs);
    }

    let tTotal = 0;
    let tRecords = [];
    let wTotal = 0;
    
    logs.forEach(log => {
      // 强制转成数字进行数学加法，杜绝字符串拼接 Bug
      const currentAmount = parseInt(log.amount) || 0; 

      wTotal += currentAmount;
      if (log.date === todayStr) {
        tTotal += currentAmount;
        tRecords.unshift(log); 
      }
    });
    
    let title = '胃口平平 🐾';
    if (tTotal >= 50 && tTotal <= 100) title = '健康干饭 🌟';
    else if (tTotal > 100) title = '超级大胃王 👑';

    this.setData({
      todayTotal: tTotal,
      weekTotal: wTotal,
      todayRecords: tRecords,
      foodieTitle: title
    });
  },

  addFeedingRecord(weight, type) {
    const now = new Date();
    const todayStr = `${now.getFullYear()}-${now.getMonth()+1}-${now.getDate()}`;
    const timeStr = `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}`;
    
    let logs = wx.getStorageSync('feed_logs') || [];
    logs.push({
      id: Date.now(),
      date: todayStr,
      time: timeStr,
      type: type,
      amount: parseInt(weight) || 0 // 确保存入的也是数字
    });
    wx.setStorageSync('feed_logs', logs);
    this.loadFeedingRecords(); 
  },

  reconnectCamera() {
    const that = this;
    wx.showLoading({ title: '正在连接...' });
    if (this.data.socketTask) this.data.socketTask.close();

    const task = wx.connectSocket({
      url: config.maixcam.websocketUrl,
      success: () => { console.log("WebSocket 发出"); }
    });
    
    task.onOpen(() => {
      wx.hideLoading();
      wx.showToast({ title: '信号已连通', icon: 'success' });
      that.setData({ videoSrc: 'ready' }); 
    });
    
    task.onMessage((res) => {
      try {
        const data = JSON.parse(res.data);
        if (data.image) {
          that.setData({
            videoSrc: 'data:image/jpeg;base64,' + data.image,
            petLabel: data.pet || ''
          });
        }
      } catch (e) {}
    });

    task.onClose(() => { that.setData({ videoSrc: '' }); });
    task.onError((err) => {
      that.setData({ videoSrc: '' });
      wx.showToast({ title: '摄像头掉线啦', icon: 'none' });
    });
    
    this.setData({ socketTask: task });
  },

  updateClock() {
    const now = new Date();
    this.setData({ currentTime: `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}` });
  },

  fetchCloudData() {
    const that = this;
    wx.request({
      url: `${config.onenet.apiBaseUrl}/thingmodel/query-device-property?product_id=${config.onenet.productId}&device_name=${config.onenet.deviceName}`,
      method: 'GET',
      header: { 'Authorization': config.onenet.authorization },
      success(res) {
        if (res.data && res.data.code === 0) {
          const props = res.data.data;
          let updateObj = {};
          
          let newEatCnt = null;
          let newEatTime = null;
          let newEatWeight = null;
          
          props.forEach(item => {
            if (item.identifier === 'tem') updateObj.temperature = item.value;
            if (item.identifier === 'wet') updateObj.humidity = item.value;
            if (item.identifier === 'wei') updateObj.foodLevel = item.value;
            
            // 精准捕获这三个 int32 物模型
            if (item.identifier === 'eat_cnt') newEatCnt = parseInt(item.value);
            if (item.identifier === 'eat_time') newEatTime = parseInt(item.value);
            if (item.identifier === 'eat_weight') newEatWeight = parseInt(item.value);
          });
          
          that.setData(updateObj);

          // 只要云端成功返回了这三个数据，就扔给处理中心判断
          if (newEatCnt !== null && newEatTime !== null && newEatWeight !== null) {
            that.processCloudHealthData(newEatCnt, newEatTime, newEatWeight);
          }
        }
      }
    });
  },

  processCloudHealthData(cnt, time, weight) {
    // 拿出手机本地记忆的“上一次干饭序号”
    const lastProcessedCnt = wx.getStorageSync('last_eat_cnt') || -1;
    
    // 如果云端传来的干饭次数没增加，说明是一条旧数据，直接无视！
    if (cnt === lastProcessedCnt) return; 

    const now = new Date();
    const todayStr = `${now.getFullYear()}-${now.getMonth()+1}-${now.getDate()}`;
    const timeStr = `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}`;
    
    let logs = wx.getStorageSync('feed_logs') || [];
    
    // 写入新记录到手账里
    logs.unshift({
      id: Date.now(),
      date: todayStr,
      time: timeStr,
      type: '智能监测', 
      duration: time,
      amount: weight || 0
    });

    if (logs.length > 50) logs = logs.slice(0, 50);

    // 存储更新后的手账，并把最新的“干饭序号”存入缓存盖戳
    wx.setStorageSync('feed_logs', logs);
    wx.setStorageSync('last_eat_cnt', cnt); 
    
    this.loadFeedingRecords(); // 刷新页面 UI
    
    wx.showToast({
      title: '检测到干饭!',
      icon: 'success',
      duration: 2000
    });
  },

  feedNow() {
    if (this.data.isFeeding) return;
    const that = this;
    wx.showActionSheet({
      itemList: ['🐱 喂猫咪 (20g)', '🐶 喂狗狗 (50g)', '⚙️ 试出粮 (5g)'],
      success(res) {
        let weight = 0;
        if (res.tapIndex === 0) weight = 20;
        else if (res.tapIndex === 1) weight = 50;
        else if (res.tapIndex === 2) weight = 5;
        that.sendFeedCommand(weight);
      }
    });
  },

  sendFeedCommand(weight) {
    const that = this;
    this.setData({ isFeeding: true });
    wx.showLoading({ title: '呼叫粮仓...' });
    wx.request({
      url: `${config.onenet.apiBaseUrl}/thingmodel/set-device-property`,
      method: 'POST',
      header: { 
        'Authorization': config.onenet.authorization,
        'Content-Type': 'application/json'
      },
      data: {
        "product_id": config.onenet.productId,
        "device_name": config.onenet.deviceName,
        "params": { "feed_amount": weight, "feed_cmd": 1 }
      },
      success(res) {
        wx.hideLoading();
        if (res.data && res.data.code === 0) {
          that.setData({ isFeeding: false, showSuccess: true });
          setTimeout(() => that.setData({ showSuccess: false }), 3000);
          wx.showToast({ title: `已安排 ${weight}g`, icon: 'success' });
          that.addFeedingRecord(weight, '手动加餐');
        } else {
          that.setData({ isFeeding: false });
          wx.showToast({ title: '出粮失败', icon: 'error' });
        }
      },
      fail() {
        wx.hideLoading();
        that.setData({ isFeeding: false });
        wx.showToast({ title: '请求失败', icon: 'none' });
      }
    });
  },

  switchTab(e) { this.setData({ activeTab: e.currentTarget.dataset.tab }); },
  openRecords() { this.setData({ showRecords: true }); },
  closeRecords() { this.setData({ showRecords: false }); },
  openDevice() { this.setData({ showDevice: true }); },
  closeDevice() { this.setData({ showDevice: false }); },
  
  sendScheduleToHardware(scheduleItem) {
    const timeParts = scheduleItem.time.split(':');
    const hour = parseInt(timeParts[0]);
    const minute = parseInt(timeParts[1]);
    const isEnabled = scheduleItem.enabled ? 1 : 0;
    wx.request({
      url: `${config.onenet.apiBaseUrl}/thingmodel/set-device-property`,
      method: 'POST',
      header: { 
        'Authorization': config.onenet.authorization,
        'Content-Type': 'application/json'
      },
      data: {
        "product_id": config.onenet.productId,
        "device_name": config.onenet.deviceName,
        "params": {
          "sch_id": scheduleItem.id,
          "sch_h": hour,
          "sch_m": minute,
          "sch_w": scheduleItem.amount,
          "sch_en": isEnabled
        }
      },
      success(res) {
        if (res.data && res.data.code === 0) {
          console.log(`计划 ${scheduleItem.id} 同步成功`);
        } else {
          wx.showToast({ title: '设备同步失败', icon: 'none' });
        }
      },
      fail() {
        wx.showToast({ title: '网络请求失败', icon: 'none' });
      }
    });
  },

  toggleSchedule(e) {
    const id = e.currentTarget.dataset.id;
    let targetSchedule = null;
    const list = this.data.schedules.map(s => { 
      if (s.id === id) {
        s.enabled = !s.enabled; 
        targetSchedule = s;
      }
      return s; 
    });
    this.setData({ schedules: list });

    if (targetSchedule) {
      this.sendScheduleToHardware(targetSchedule);
    }
  },

  addNewSchedule() {
    this.setData({
      showScheduleModal: true,
      editingSchedule: null,
      tempTime: '08:00',
      tempAmount: 30
    });
  },

  editSchedule(e) {
    const item = e.currentTarget.dataset.item;
    this.setData({
      showScheduleModal: true,
      editingSchedule: item,
      tempTime: item.time,
      tempAmount: item.amount
    });
  },

  closeScheduleModal() {
    this.setData({ showScheduleModal: false });
  },

  onTimeChange(e) {
    this.setData({ tempTime: e.detail.value });
  },

  onAmountChange(e) {
    this.setData({ tempAmount: e.detail.value });
  },

  saveSchedule() {
    let list = [...this.data.schedules];
    let currentSchedule = null;
    if (this.data.editingSchedule) {
      const index = list.findIndex(s => s.id === this.data.editingSchedule.id);
      if (index > -1) {
        list[index].time = this.data.tempTime;
        list[index].amount = this.data.tempAmount;
        currentSchedule = list[index];
      }
    } else {
      if (list.length >= 5) {
        wx.showToast({ title: '最多只能存5个计划', icon: 'none' });
        return;
      }
      
      let newId = 0;
      const existingIds = list.map(s => s.id);
      while (existingIds.includes(newId)) {
        newId++;
      }

      currentSchedule = {
        id: newId,
        time: this.data.tempTime,
        amount: this.data.tempAmount,
        frequency: '每天',
        enabled: true
      };
      list.push(currentSchedule);
    }

    list.sort((a, b) => a.time.localeCompare(b.time));

    this.setData({ 
      schedules: list, 
      showScheduleModal: false 
    });
    wx.showToast({ title: '保存成功', icon: 'success' });

    if (currentSchedule) {
      this.sendScheduleToHardware(currentSchedule);
    }
  },

  deleteSchedule() {
    const that = this;
    wx.showModal({
      title: '删除计划',
      content: '确定要删除这个喂食计划吗？',
      confirmColor: '#dc2626',
      success(res) {
        if (res.confirm) {
          const scheduleToDelete = that.data.editingSchedule;
          
          const list = that.data.schedules.filter(s => s.id !== scheduleToDelete.id);
          that.setData({ 
            schedules: list, 
            showScheduleModal: false 
          });
          wx.showToast({ title: '已删除', icon: 'success' });

          scheduleToDelete.enabled = false;
          that.sendScheduleToHardware(scheduleToDelete);
        }
      }
    });
  }
})
