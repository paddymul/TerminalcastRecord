
(add-hook 'post-command-hook
	  '(lambda ()
             (set-window-point (car(remove-if-not 
              '(lambda (test-win) 
                 (eq (current-buffer)  (window-buffer test-win)))
              (window-list (car 
                            (remove-if
                             '(lambda (frm)
                                (eq (frame-selected-window frm) (selected-window)))
                             (frame-list)))))) 
                               (point) )))


             (message (point))))
             (message (format "point %d" (point)))))
             (set-window-point (cadr (window-list)) (point))))
(require 'cl)



(defun paddy-current-frame ()
  (window-configuration-frame (current-window-configuration)))
(defvar paddy-followed-frame (paddy-current-frame))
(defvar paddy-following-frame (make-frame))

(setf paddy-following-frame (make-frame))

(defun paddy-set-window-configuration (_winsav-wc _frame)
  (let ((_old-frame (paddy-current-frame)))
    (select-frame _frame)
    (delete-other-windows)
    (winsav-put-window-tree _wt (winsav-upper-left-window))
    (select-frame _old-frame)))

 (paddy-set-window-configuration (winsav-get-window-tree (paddy-current-frame)) paddy-following-frame)
(setf _wt (winsav-get-window-tree (paddy-current-frame)))







(eq paddy-followed-frame paddy-following-frame) ;; should be false

(defun paddy-get-frame-configuration (_paddy-outer-frame)
  (car (remove-if-not 
        '(lambda (_frm)
           (eq _paddy-outer-frame (car _frm))) (cdr (current-frame-configuration)))))

(cdr (current-frame-configuration)

(defun paddy-set-frame-configuration (_frame _configuration) 
;  (set-frame-configuration
  (cons (car (current-frame-configuration))
   (substitute
    (paddy-get-frame-configuration _frame)
    (cons  _frame (cdr _configuration))
    (cdr (current-frame-configuration)))))

(message (format "%S" (paddy-set-frame-configuration paddy-following-frame       (paddy-get-frame-configuration paddy-followed-frame))))

(defun paddy-set-frame-window-configuration (_frame _configuration) 
 ; (let ((_old-frame (paddy-current-frame)))
    (select-frame _frame)
    (set-window-configuration _configuration)
    ;(select-frame _old-frame)
)


(setq f-w-cfg (current-window-configuration))
(set-window-configuration f-w-cfg)

(paddy-set-frame-window-configuration paddy-following-frame (current-window-configuration))
 paddy-followed-frame))
;  (set-frame-configuration
  (cons (car (current-frame-configuration))
   (substitute
    (paddy-get-frame-configuration _frame)
    (cons  _frame (cdr _configuration))
    (cdr (current-frame-configuration)))))





(current-window-configuration paddy-followed-frame)



(caddr (car (cdr (current-frame-configuration))))

(add-hook 'set-window-configuration-hook
          '(lambda ()
             (when (eq (paddy-current-frame) paddy-followed-frame)
               
             



 ) ))

(cdr (frame-list))
 
(length (remove-if
 'frame-live-p (frame-list)))

(length

(length (frame-list))

(walk-windows)


(defun paddy-message (args)
  (message (format "%S" args)))
(defun save-frame-configuration ()
  (let 
      ((fs)(f))
    (setq fs (loop for c in (cdr (current-frame-configuration)) 
          collect (progn 
                    (setq f (cadr c))
                    (reduce (lambda (acc a) (if (find (symbol-name (car a)) '("top" "left" "width" "height") :test 'equal) (cons a acc) acc)) f :initial-value 'nil)
    )))
    ;; fs contains a list of attribs for each frame
    (save-window-excursion
      (find-file "~/.e_last_frame_config.el")
      (erase-buffer)
      (print (cons 'version 1) (current-buffer))
      (print fs (current-buffer))
      (save-buffer)
      )
    ))
(save-frame-configuration)

(defun load-frame-configuration ()
  "load the last saved frame configuration, if it exists"
  (let
      ((v) (fs))
    (if (file-exists-p "~/.e_last_frame_config.el")
        (save-window-excursion
          (find-file "~/.e_last_frame_config.el")
          (beginning-of-buffer)
          (setq v (read (current-buffer)))
          (if (not (and (equal 'version (car v)) (= 1 (cdr v))))
              (error "version %i not understood" (cdr v)))
          (setq fs (read (current-buffer)))
          (loop for f in fs do
                (make-frame f)))
      (message "~/.e_last_frame_config.el not found. not loaded"))))
(load-frame-configuration)
(paddy-message (window-tree))
(paddy-message (car(window-tree)))

