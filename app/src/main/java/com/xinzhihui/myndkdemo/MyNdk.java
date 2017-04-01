package com.xinzhihui.myndkdemo;

/**
 * Created by zhangyipeng on 16/2/28.
 */
public class MyNdk {

    static {
        System.loadLibrary("MyLibrary");
    }

    public native String getString();

    public native String socket(String url, String head);
}