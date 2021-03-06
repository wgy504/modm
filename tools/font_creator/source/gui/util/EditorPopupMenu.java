/*
 * Copyright (c) 2010, Fabian Greif
 * Copyright (c) 2015, Kevin Laeufer
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

/*
 * Created on 17.03.2004
 *
 * To change the template for this generated file go to
 * Window&gt;Preferences&gt;Java&gt;Code Generation&gt;Code and Comments
 */
package gui.util;

import gui.CharEditor;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JMenuItem;
import javax.swing.JPopupMenu;

/**
 * @author Fabian Maximilian Thiele
 */
public class EditorPopupMenu extends JPopupMenu implements ActionListener {
	private static final long serialVersionUID = 1124433022586577855L;
	private CharEditor parent;

	public EditorPopupMenu(CharEditor parent) {
		this.parent = parent;

		JMenuItem clear = new JMenuItem("Clear");
		clear.setActionCommand("clear");
		clear.addActionListener(this);
		this.add(clear);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
	 */
	public void actionPerformed(ActionEvent e) {
		String cmd = e.getActionCommand();
		if (cmd.equals("clear")) {
			parent.clear();
			parent.repaint();
		}
	}

}
