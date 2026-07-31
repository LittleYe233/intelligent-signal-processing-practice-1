import time
import os
import cv2
import numpy as np
import pyautogui
from PIL import ImageGrab
import keyboard

# 保存截图的文件夹
OUTPUT_DIR = "captured_charts"
os.makedirs(OUTPUT_DIR, exist_ok=True)

class ChartCapturer:
    def __init__(self):
        self.chart_w = None
        self.chart_h = None
        self.saved_hashes = []
        self.chart_count = 0

    def get_image_hash(self, img_np):
        """计算图像特征，用于去重"""
        resized = cv2.resize(img_np, (32, 32))
        gray = cv2.cvtColor(resized, cv2.COLOR_BGR2GRAY)
        avg = gray.mean()
        return (gray > avg).tobytes()

    def is_duplicate(self, img_np):
        """判断是否已经保存过该图表"""
        curr_hash = self.get_image_hash(img_np)
        for h in self.saved_hashes:
            if h == curr_hash:
                return True
        return curr_hash

    def calibrate_and_run(self, region_bbox, scroll_pos):
        """
        region_bbox: (left, top, width, height) 滚动子区域的屏幕坐标范围
        """
        rx, ry, rw, rh = region_bbox
        print(f"\n[+] 正在分析区域: {region_bbox}")

        # 1. 抓取当前子区域屏幕截图
        screen = ImageGrab.grab(bbox=(rx, ry, rx + rw, ry + rh))
        img_bgr = cv2.cvtColor(np.array(screen), cv2.COLOR_RGB2BGR)

        # 2. 二值化分割：识别纯白区域 (RGB接近255, 255, 255)
        gray = cv2.cvtColor(img_bgr, cv2.COLOR_BGR2GRAY)
        _, thresh = cv2.threshold(gray, 250, 255, cv2.THRESH_BINARY)

        # 3. 寻找白色连通区域的轮廓
        contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        valid_rects = []
        for cnt in contours:
            x, y, w, h = cv2.boundingRect(cnt)
            # 过滤掉太小的噪点
            if w > 300 and h > 300:
                valid_rects.append((x, y, w, h))

        if not valid_rects:
            print("[-] 未能在该区域检测到白底图表，请确保区域内有完整的图表显示！")
            return

        # 以最常见的矩形尺寸作为标准图表的尺寸 (W, H)
        # 排序取中位数/频次最高值，防止首尾半截图表干扰
        widths = [r[2] for r in valid_rects]
        heights = [r[3] for r in valid_rects]
        self.chart_w = int(np.max(widths))
        self.chart_h = int(np.max(heights))

        print(f"[✓] 成功校准！检测到图表标准尺寸: {self.chart_w} x {self.chart_h} 像素")
        print("[+] 开始自动滚动截图，请勿移动鼠标/切换窗口...")

        # 4. 循环滚动截取
        no_new_chart_count = 0
        scroll_center_x, scroll_center_y = scroll_pos
        TITLE_OFFSET = 35 # 图表标题需要裁剪

        while True:
            # 抓取当前视角图像
            current_screen = ImageGrab.grab(bbox=(rx, ry, rx + rw, ry + rh))
            curr_bgr = cv2.cvtColor(np.array(current_screen), cv2.COLOR_RGB2BGR)
            curr_gray = cv2.cvtColor(curr_bgr, cv2.COLOR_BGR2GRAY)
            _, curr_thresh = cv2.threshold(curr_gray, 250, 255, cv2.THRESH_BINARY)

            contours, _ = cv2.findContours(curr_thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            
            found_in_this_frame = False

            for cnt in contours:
                x, y, w, h = cv2.boundingRect(cnt)
                
                # 严格匹配尺寸：允许 ±2 像素的边缘抗锯齿容差
                if abs(w - self.chart_w) <= 2 and abs(h - self.chart_h) <= 2:
                    # 精确裁剪图表（使用校准出的标准 W 和 H，保证100%尺寸统一）
                    chart_img = curr_bgr[y + TITLE_OFFSET:y + self.chart_h, x:x + self.chart_w]
                    
                    # 去重检查
                    img_hash = self.is_duplicate(chart_img)
                    if img_hash is not True:
                        self.chart_count += 1
                        file_path = os.path.join(OUTPUT_DIR, f"chart_{self.chart_count:03d}.png")
                        cv2.imwrite(file_path, chart_img)
                        self.saved_hashes.append(img_hash)
                        print(f"  -> 已保存图表 #{self.chart_count}: {file_path}")
                        found_in_this_frame = True

            if not found_in_this_frame:
                no_new_chart_count += 1
            else:
                no_new_chart_count = 0

            # 如果连续滚动都没有发现新图表，说明已滚到底部
            if no_new_chart_count >= 10:
                print(f"\n[✓] 截图完成！共截取 {self.chart_count} 张图表，保存于 `{OUTPUT_DIR}` 目录。")
                break

            # 模拟鼠标向下滚动（滚轮步长取图表高度的 10%，确保不会漏掉图表）
            pyautogui.moveTo(scroll_center_x, scroll_center_y)
            # scroll 负值表示向下滚动
            pyautogui.scroll(-int(self.chart_h * 0.2))
            time.sleep(0.3)  # 等待界面渲染和滚动停止

def main():
    print("=" * 60)
    print(" Windows 子区域图表精准自动截图工具")
    print("=" * 60)
    print("操作说明：")
    print("1. 将鼠标移动到【可滚动的灰色子区域】的【左上角】，按键盘 [ F8 ] 键")
    print("2. 将鼠标移动到【可滚动的灰色子区域】的【右下角】，按键盘 [ F9 ] 键")
    print("3. 将鼠标移动到【可滚动的灰色子区域】的【可滚动位置】，按键盘 [ F10 ] 键")
    print("4. 程序将自动识别图表尺寸并完成滚动截图")
    print("=" * 60)

    p1, p2, p3 = None, None, None

    while True:
        if keyboard.is_pressed('f8'):
            p1 = pyautogui.position()
            print(f"[✓] 标记区域左上角: {p1}")
            time.sleep(0.5)

        if keyboard.is_pressed('f9'):
            p2 = pyautogui.position()
            print(f"[✓] 标记区域右下角: {p2}")
            time.sleep(0.5)

        if keyboard.is_pressed('f10'):
            p3 = pyautogui.position()
            print(f"[✓] 标记可滚动位置: {p3}")
            time.sleep(0.5)

        if p1 and p2 and p3:
            left = min(p1.x, p2.x)
            top = min(p1.y, p2.y)
            width = abs(p2.x - p1.x)
            height = abs(p2.y - p1.y)
            
            capturer = ChartCapturer()
            capturer.calibrate_and_run((left, top, width, height), (p3.x, p3.y))
            break

        time.sleep(0.05)

if __name__ == "__main__":
    main()